#requires -version 5.1

<#
.SYNOPSIS
    MouseFlip - Toggle mouse button configuration via system tray
.DESCRIPTION
    A system tray application that allows quick toggling between right-handed
    and left-handed mouse configurations. Features auto-switch based on display
    configuration and startup with Windows option.
.NOTES
    Version: 1.0
    Author: MouseFlip
#>

# Ensure we're running with proper UI thread apartment state
[Threading.Thread]::CurrentThread.SetApartmentState('STA') | Out-Null

# Load required assemblies
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

# Define Win32 API calls via P/Invoke
Add-Type @"
using System;
using System.Runtime.InteropServices;

public class Win32 {
    [DllImport("user32.dll")]
    public static extern bool SwapMouseButton(bool fSwap);

    [DllImport("user32.dll")]
    public static extern int GetSystemMetrics(int nIndex);

    public const int SM_SWAPBUTTON = 23;
    public const int SM_CMONITORS = 80;
}
"@

# Script configuration
$script:AppName = "MouseFlip"
$script:AppVersion = "1.0"
$script:RegistryPath = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Run"
$script:SettingsPath = "HKCU:\Software\MouseFlip"
$script:ScriptPath = $PSCommandPath
$script:ScriptDir = Split-Path -Parent $PSCommandPath

# Icon paths (relative to script location)
$script:IconPaths = @{
    App = Join-Path $script:ScriptDir "resources\app_icon.ico"
    Right = Join-Path $script:ScriptDir "resources\icon_right.ico"
    Left = Join-Path $script:ScriptDir "resources\icon_left.ico"
}

# Global state
$script:NotifyIcon = $null
$script:ContextMenu = $null
$script:AutoSwitchTimer = $null
$script:LastDisplayState = $null
$script:LastClickTime = 0
$script:ClickCount = 0

#region Helper Functions

function Get-MouseButtonState {
    <#
    .SYNOPSIS
        Get current mouse button swap state
    .OUTPUTS
        [bool] True if swapped (left-handed), False if normal (right-handed)
    #>
    return ([Win32]::GetSystemMetrics([Win32]::SM_SWAPBUTTON) -ne 0)
}

function Set-MouseButtonState {
    <#
    .SYNOPSIS
        Set mouse button swap state
    .PARAMETER LeftHanded
        If $true, swaps buttons for left-handed mode
    #>
    param([bool]$LeftHanded)
    [Win32]::SwapMouseButton($LeftHanded) | Out-Null
}

function Toggle-MouseButtons {
    <#
    .SYNOPSIS
        Toggle mouse button configuration
    #>
    $currentState = Get-MouseButtonState
    Set-MouseButtonState -LeftHanded (!$currentState)
}

function Get-CurrentIconPath {
    <#
    .SYNOPSIS
        Get icon path based on current mouse state
    #>
    $isLeftHanded = Get-MouseButtonState
    return ($isLeftHanded ? $script:IconPaths.Left : $script:IconPaths.Right)
}

function Update-TrayIcon {
    <#
    .SYNOPSIS
        Update tray icon to reflect current state
    #>
    $iconPath = Get-CurrentIconPath
    if (Test-Path $iconPath) {
        $script:NotifyIcon.Icon = [System.Drawing.Icon]::new($iconPath)
    }
}

function Get-MonitorCount {
    <#
    .SYNOPSIS
        Get number of active monitors
    #>
    return [Win32]::GetSystemMetrics([Win32]::SM_CMONITORS)
}

function Test-LaptopScreenOnly {
    <#
    .SYNOPSIS
        Check if only laptop screen is active (no external displays)
    #>
    return (Get-MonitorCount -eq 1)
}

function Get-StartupEnabled {
    <#
    .SYNOPSIS
        Check if application is set to start with Windows
    #>
    try {
        $value = Get-ItemProperty -Path $script:RegistryPath -Name $script:AppName -ErrorAction SilentlyContinue
        return ($null -ne $value)
    } catch {
        return $false
    }
}

function Set-StartupEnabled {
    <#
    .SYNOPSIS
        Enable or disable startup with Windows
    #>
    param([bool]$Enable)

    try {
        if ($Enable) {
            # Build PowerShell command that will run this script
            $command = "powershell.exe -WindowStyle Hidden -ExecutionPolicy Bypass -File `"$script:ScriptPath`""
            Set-ItemProperty -Path $script:RegistryPath -Name $script:AppName -Value $command -Type String
        } else {
            Remove-ItemProperty -Path $script:RegistryPath -Name $script:AppName -ErrorAction SilentlyContinue
        }
        return $true
    } catch {
        return $false
    }
}

function Get-AutoSwitchEnabled {
    <#
    .SYNOPSIS
        Check if auto-switch feature is enabled (default: true)
    #>
    try {
        if (!(Test-Path $script:SettingsPath)) {
            return $true  # Default enabled
        }
        $value = Get-ItemProperty -Path $script:SettingsPath -Name "AutoSwitch" -ErrorAction SilentlyContinue
        if ($null -eq $value) {
            return $true  # Default enabled
        }
        return ($value.AutoSwitch -ne 0)
    } catch {
        return $true  # Default enabled
    }
}

function Set-AutoSwitchEnabled {
    <#
    .SYNOPSIS
        Enable or disable auto-switch feature
    #>
    param([bool]$Enable)

    try {
        if (!(Test-Path $script:SettingsPath)) {
            New-Item -Path $script:SettingsPath -Force | Out-Null
        }
        Set-ItemProperty -Path $script:SettingsPath -Name "AutoSwitch" -Value ([int]$Enable) -Type DWord
        return $true
    } catch {
        return $false
    }
}

function Start-AutoSwitchMonitoring {
    <#
    .SYNOPSIS
        Start monitoring display configuration for auto-switch
    #>
    if ($null -eq $script:AutoSwitchTimer) {
        $script:AutoSwitchTimer = New-Object System.Windows.Forms.Timer
        $script:AutoSwitchTimer.Interval = 2000  # 2 seconds
        $script:AutoSwitchTimer.Add_Tick({ Invoke-AutoSwitch })
        $script:AutoSwitchTimer.Start()

        # Initialize last state to opposite of current to force initial switch
        $script:LastDisplayState = !(Test-LaptopScreenOnly)
        Invoke-AutoSwitch
    }
}

function Stop-AutoSwitchMonitoring {
    <#
    .SYNOPSIS
        Stop monitoring display configuration
    #>
    if ($null -ne $script:AutoSwitchTimer) {
        $script:AutoSwitchTimer.Stop()
        $script:AutoSwitchTimer.Dispose()
        $script:AutoSwitchTimer = $null
    }
}

function Invoke-AutoSwitch {
    <#
    .SYNOPSIS
        Check display state and apply appropriate mouse configuration
    #>
    $laptopScreenOnly = Test-LaptopScreenOnly

    # Only switch if state has changed
    if ($laptopScreenOnly -ne $script:LastDisplayState) {
        $script:LastDisplayState = $laptopScreenOnly

        if ($laptopScreenOnly) {
            # Laptop screen only → Right-handed
            Set-MouseButtonState -LeftHanded $false
        } else {
            # External display connected or lid closed → Left-handed
            Set-MouseButtonState -LeftHanded $true
        }

        Update-TrayIcon
    }
}

#endregion

#region UI Functions

function Show-ContextMenu {
    <#
    .SYNOPSIS
        Show context menu at cursor position
    #>
    $isLeftHanded = Get-MouseButtonState

    # Update checkmarks
    $script:ContextMenu.Items["RightHanded"].Checked = !$isLeftHanded
    $script:ContextMenu.Items["LeftHanded"].Checked = $isLeftHanded

    # Show menu (will appear at current cursor position)
}

function Show-AboutDialog {
    <#
    .SYNOPSIS
        Show About dialog
    #>
    $form = New-Object System.Windows.Forms.Form
    $form.Text = "About $script:AppName"
    $form.Size = New-Object System.Drawing.Size(400, 250)
    $form.StartPosition = "CenterScreen"
    $form.FormBorderStyle = "FixedDialog"
    $form.MaximizeBox = $false
    $form.MinimizeBox = $false

    # Icon
    if (Test-Path $script:IconPaths.App) {
        $form.Icon = [System.Drawing.Icon]::new($script:IconPaths.App)
    }

    # Message
    $label = New-Object System.Windows.Forms.Label
    $label.Location = New-Object System.Drawing.Point(20, 20)
    $label.Size = New-Object System.Drawing.Size(360, 150)
    $label.Text = @"
$script:AppName v$script:AppVersion

Quickly toggle mouse button configuration
between right-handed and left-handed modes.

Double-click the tray icon with either button to flip.
Right-click for menu.

PowerShell version - Cross-platform compatible
"@
    $form.Controls.Add($label)

    # OK Button
    $okButton = New-Object System.Windows.Forms.Button
    $okButton.Location = New-Object System.Drawing.Point(150, 180)
    $okButton.Size = New-Object System.Drawing.Size(100, 30)
    $okButton.Text = "OK"
    $okButton.DialogResult = [System.Windows.Forms.DialogResult]::OK
    $form.Controls.Add($okButton)
    $form.AcceptButton = $okButton

    $form.ShowDialog() | Out-Null
    $form.Dispose()
}

function Show-OptionsDialog {
    <#
    .SYNOPSIS
        Show Options dialog
    #>
    $form = New-Object System.Windows.Forms.Form
    $form.Text = "$script:AppName Options"
    $form.Size = New-Object System.Drawing.Size(450, 220)
    $form.StartPosition = "CenterScreen"
    $form.FormBorderStyle = "FixedDialog"
    $form.MaximizeBox = $false
    $form.MinimizeBox = $false

    # Icon
    if (Test-Path $script:IconPaths.App) {
        $form.Icon = [System.Drawing.Icon]::new($script:IconPaths.App)
    }

    # Startup checkbox
    $startupCheck = New-Object System.Windows.Forms.CheckBox
    $startupCheck.Location = New-Object System.Drawing.Point(20, 20)
    $startupCheck.Size = New-Object System.Drawing.Size(400, 30)
    $startupCheck.Text = "Start $script:AppName when Windows starts"
    $startupCheck.Checked = Get-StartupEnabled
    $form.Controls.Add($startupCheck)

    # Auto-switch checkbox
    $autoSwitchCheck = New-Object System.Windows.Forms.CheckBox
    $autoSwitchCheck.Location = New-Object System.Drawing.Point(20, 60)
    $autoSwitchCheck.Size = New-Object System.Drawing.Size(400, 30)
    $autoSwitchCheck.Text = "Auto-switch based on display configuration (Enabled by default)"
    $autoSwitchCheck.Checked = Get-AutoSwitchEnabled
    $form.Controls.Add($autoSwitchCheck)

    # Description label
    $descLabel = New-Object System.Windows.Forms.Label
    $descLabel.Location = New-Object System.Drawing.Point(40, 90)
    $descLabel.Size = New-Object System.Drawing.Size(380, 50)
    $descLabel.Text = "Right-handed when using laptop screen only`nLeft-handed when external display connected or lid closed"
    $descLabel.ForeColor = [System.Drawing.Color]::Gray
    $form.Controls.Add($descLabel)

    # OK Button
    $okButton = New-Object System.Windows.Forms.Button
    $okButton.Location = New-Object System.Drawing.Point(240, 150)
    $okButton.Size = New-Object System.Drawing.Size(90, 30)
    $okButton.Text = "OK"
    $okButton.DialogResult = [System.Windows.Forms.DialogResult]::OK
    $form.Controls.Add($okButton)

    # Cancel Button
    $cancelButton = New-Object System.Windows.Forms.Button
    $cancelButton.Location = New-Object System.Drawing.Point(340, 150)
    $cancelButton.Size = New-Object System.Drawing.Size(90, 30)
    $cancelButton.Text = "Cancel"
    $cancelButton.DialogResult = [System.Windows.Forms.DialogResult]::Cancel
    $form.Controls.Add($cancelButton)

    $form.AcceptButton = $okButton
    $form.CancelButton = $cancelButton

    $result = $form.ShowDialog()

    if ($result -eq [System.Windows.Forms.DialogResult]::OK) {
        # Apply startup setting
        if (!(Set-StartupEnabled $startupCheck.Checked)) {
            [System.Windows.Forms.MessageBox]::Show(
                "Failed to update startup settings. Please check your permissions.",
                "Error",
                [System.Windows.Forms.MessageBoxButtons]::OK,
                [System.Windows.Forms.MessageBoxIcon]::Error
            )
        }

        # Apply auto-switch setting
        $autoSwitchEnabled = $autoSwitchCheck.Checked
        if (!(Set-AutoSwitchEnabled $autoSwitchEnabled)) {
            [System.Windows.Forms.MessageBox]::Show(
                "Failed to update auto-switch settings. Please check your permissions.",
                "Error",
                [System.Windows.Forms.MessageBoxButtons]::OK,
                [System.Windows.Forms.MessageBoxIcon]::Error
            )
        } else {
            # Start or stop monitoring
            if ($autoSwitchEnabled) {
                Start-AutoSwitchMonitoring
            } else {
                Stop-AutoSwitchMonitoring
            }
        }
    }

    $form.Dispose()
}

#endregion

#region Tray Icon Setup

function Initialize-TrayIcon {
    <#
    .SYNOPSIS
        Create and configure the system tray icon
    #>

    # Create NotifyIcon
    $script:NotifyIcon = New-Object System.Windows.Forms.NotifyIcon
    $script:NotifyIcon.Text = "$script:AppName - Double-click either button to flip"
    $script:NotifyIcon.Visible = $true

    # Set initial icon
    Update-TrayIcon

    # Create context menu
    $script:ContextMenu = New-Object System.Windows.Forms.ContextMenuStrip

    # Right-handed menu item
    $rightHandedItem = New-Object System.Windows.Forms.ToolStripMenuItem
    $rightHandedItem.Text = "Right-handed"
    $rightHandedItem.Name = "RightHanded"
    $rightHandedItem.Add_Click({
        Set-MouseButtonState -LeftHanded $false
        Update-TrayIcon
    })
    $script:ContextMenu.Items.Add($rightHandedItem) | Out-Null

    # Left-handed menu item
    $leftHandedItem = New-Object System.Windows.Forms.ToolStripMenuItem
    $leftHandedItem.Text = "Left-handed"
    $leftHandedItem.Name = "LeftHanded"
    $leftHandedItem.Add_Click({
        Set-MouseButtonState -LeftHanded $true
        Update-TrayIcon
    })
    $script:ContextMenu.Items.Add($leftHandedItem) | Out-Null

    # Separator
    $script:ContextMenu.Items.Add((New-Object System.Windows.Forms.ToolStripSeparator)) | Out-Null

    # Options menu item
    $optionsItem = New-Object System.Windows.Forms.ToolStripMenuItem
    $optionsItem.Text = "Options..."
    $optionsItem.Add_Click({ Show-OptionsDialog })
    $script:ContextMenu.Items.Add($optionsItem) | Out-Null

    # About menu item
    $aboutItem = New-Object System.Windows.Forms.ToolStripMenuItem
    $aboutItem.Text = "About"
    $aboutItem.Add_Click({ Show-AboutDialog })
    $script:ContextMenu.Items.Add($aboutItem) | Out-Null

    # Exit menu item
    $exitItem = New-Object System.Windows.Forms.ToolStripMenuItem
    $exitItem.Text = "Exit"
    $exitItem.Add_Click({
        Stop-AutoSwitchMonitoring
        $script:NotifyIcon.Visible = $false
        $script:NotifyIcon.Dispose()
        [System.Windows.Forms.Application]::Exit()
    })
    $script:ContextMenu.Items.Add($exitItem) | Out-Null

    # Assign context menu
    $script:NotifyIcon.ContextMenuStrip = $script:ContextMenu

    # Handle mouse clicks for double-click detection
    $script:NotifyIcon.Add_MouseDown({
        param($sender, $e)

        $currentTime = [Environment]::TickCount

        # Check if this is a double-click (within 500ms)
        if (($currentTime - $script:LastClickTime) -lt 500) {
            # Double-click detected
            Toggle-MouseButtons
            Update-TrayIcon
            $script:ClickCount = 0
        } else {
            $script:ClickCount = 1
        }

        $script:LastClickTime = $currentTime
    })

    # Handle opening of context menu to update checkmarks
    $script:ContextMenu.Add_Opening({
        Show-ContextMenu
    })
}

#endregion

#region Main Execution

function Start-MouseFlip {
    <#
    .SYNOPSIS
        Main entry point
    #>

    # Check if icons exist
    $missingIcons = @()
    foreach ($iconType in $script:IconPaths.Keys) {
        if (!(Test-Path $script:IconPaths[$iconType])) {
            $missingIcons += $iconType
        }
    }

    if ($missingIcons.Count -gt 0) {
        [System.Windows.Forms.MessageBox]::Show(
            "Missing icon files: $($missingIcons -join ', ')`n`nPlease ensure the 'resources' folder with .ico files is in the same directory as this script.",
            "Error - Missing Icons",
            [System.Windows.Forms.MessageBoxButtons]::OK,
            [System.Windows.Forms.MessageBoxIcon]::Error
        )
        return
    }

    # Initialize tray icon
    Initialize-TrayIcon

    # Start auto-switch if enabled
    if (Get-AutoSwitchEnabled) {
        Start-AutoSwitchMonitoring
    }

    # Create application context and run
    $appContext = New-Object System.Windows.Forms.ApplicationContext
    [System.Windows.Forms.Application]::Run($appContext)
}

# Run the application
Start-MouseFlip

#endregion
