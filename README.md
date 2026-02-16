# Primary

A lightweight Windows system tray application that allows you to quickly toggle mouse button configuration between right-handed and left-handed modes.

**Cross-compiled on Linux using MinGW-w64 for Windows targets.**

## Features

- **Quick Toggle**: Double-click the tray icon with either mouse button to instantly swap mouse buttons
- **Visual Feedback**: Icon changes to reflect current mouse orientation
- **Auto-Switch**: Automatically switches mouse orientation based on external mouse detection
  - Right-handed when using trackpad only (no external mouse)
  - Left-handed when external mouse is connected
- **Startup with Windows**: Optional setting to launch automatically when Windows starts
- **Context Menu**: Right-click for menu with orientation options, settings, about dialog, and exit
- **Lightweight**: Minimal resource usage, runs silently in system tray
- **Native**: Pure Win32 API, no external dependencies

## Use Cases

- Ambidextrous users who switch hands frequently
- Shared computers with users who have different preferences
- Accessibility needs requiring quick mouse configuration changes
- Testing applications with different mouse button configurations

## Requirements

### Runtime
- Windows 10 or Windows 11
- No additional runtime dependencies required

### Building
- Ubuntu/Debian Linux (or WSL)
- MinGW-w64 cross-compiler toolchain
- GCC/G++ compiler

## Building from Source

### Installing MinGW-w64

On Ubuntu/Debian:
```bash
sudo apt update
sudo apt install mingw-w64 g++
```

On other distributions, install the equivalent `mingw-w64` package.

### Building

1. **Clone or navigate to the project directory**
   ```bash
   cd primary
   ```

2. **Run the build script**
   ```bash
   ./build.sh
   ```

   The script will:
   - Check for MinGW-w64 installation
   - Compile resources with `windres`
   - Compile and link with `x86_64-w64-mingw32-g++`
   - Create `Primary.exe` in the project root

### Manual Build Commands

If you need to build manually:

```bash
# Compile resources
x86_64-w64-mingw32-windres resources/primary.rc -O coff -o resources/primary.res

# Compile and link
x86_64-w64-mingw32-g++ -std=c++11 -Wall -Wextra -DUNICODE -D_UNICODE \
     -mwindows \
     src/primary.cpp \
     resources/primary.res \
     -o Primary.exe \
     -luser32 -lshell32 -static-libgcc -static-libstdc++
```

## Usage

### Running the Application

1. Transfer `Primary.exe` to your Windows machine (or run via Wine on Linux)
2. Double-click `Primary.exe`
3. The application icon will appear in your system tray (notification area)

### Operations

- **Double-click tray icon** (with either mouse button): Toggle between right-handed and left-handed mouse modes
- **Right-click tray icon**: Open context menu with the following options:
  - **Right-handed**: Set mouse to right-handed mode (standard)
  - **Left-handed**: Set mouse to left-handed mode (buttons swapped)
  - **Options...**: Configure startup behavior and other settings
  - **About**: Display application information
  - **Exit**: Close the application and remove tray icon

### Options Dialog

Access the Options dialog by right-clicking the tray icon and selecting "Options...":

**Startup Settings:**
- **Start Primary when Windows starts**: Enable this checkbox to automatically launch Primary when you log in to Windows. The setting is stored in the Windows registry (HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run)

**Auto-Switch Settings:**
- **Auto-switch based on external mouse detection**: (**Enabled by default**) Automatically switches mouse orientation based on connected pointing devices:
  - **Right-handed mode**: When using only the trackpad (no external mouse detected)
  - **Left-handed mode**: When an external mouse is connected
  - The application monitors connected input devices every 2 seconds and switches automatically
  - Useful for users who prefer different orientations when using external mouse vs. trackpad
  - **To disable**: Simply uncheck this box in the Options dialog if you prefer manual control
  - Settings stored in HKEY_CURRENT_USER\Software\Primary

**Mouse Device Configuration:**
- **Currently detected devices**: Shows the real-time count of mouse devices detected by the system
- **Base device count (undocked)**: Configure how many mouse devices are present in your bare configuration (typically 1 for a single trackpad, but some systems have 2 built-in mouse devices)
  - Devices above this count are considered external mice
  - This setting allows the auto-switch feature to work correctly on systems with multiple built-in pointing devices
  - Default value is 1
  - Stored in HKEY_CURRENT_USER\Software\Primary

### What Gets Changed

When you flip the mouse orientation:
- Left mouse button and right mouse button functions are swapped system-wide
- The change persists until you flip it back or restart Windows
- The tray icon updates to reflect the current state

## Project Structure

```
primary/
├── src/
│   └── primary.cpp          # Main application source
├── resources/
│   ├── primary.rc           # Resource definition file
│   ├── resource.h             # Resource ID constants
│   ├── app_icon.ico           # Application icon
│   ├── icon_right.ico         # Right-handed mouse icon
│   └── icon_left.ico          # Left-handed mouse icon
├── build.sh                   # Build script (Linux)
└── README.md                  # This file
```

## Technical Details

### Architecture
- Pure Win32 API application
- Window class with hidden window for message processing
- System tray integration via `Shell_NotifyIcon`
- Popup menu for user interaction
- State synchronization with system settings

### Key Windows APIs Used
- `Shell_NotifyIcon()`: System tray icon management
- `SwapMouseButton()`: Toggle mouse button configuration
- `GetSystemMetrics(SM_SWAPBUTTON)`: Query current mouse state
- `GetRawInputDeviceList()`: Enumerate connected input devices for external mouse detection
- `CreatePopupMenu()`, `TrackPopupMenu()`: Context menu
- Standard window management APIs

### Cross-Compilation
- Built on Linux using MinGW-w64 (Minimalist GNU for Windows)
- Targets Windows x64 platform
- Static linking for portability (no DLL dependencies)
- GCC/G++ compiler with Windows headers

### State Management
The application always queries the actual system state rather than maintaining internal state. This ensures the icon accurately reflects the current mouse configuration even if changed by other means (Control Panel, Settings app, other applications).

## Compiler Flags Explained

- `-std=c++11`: Use C++11 standard
- `-Wall -Wextra`: Enable comprehensive warnings
- `-DUNICODE -D_UNICODE`: Build with Unicode support
- `-mwindows`: Build as Windows GUI application (no console)
- `-static-libgcc -static-libstdc++`: Static linking for portability
- `-luser32 -lshell32`: Link Windows system libraries

## Troubleshooting

### Build Issues

**MinGW-w64 not found**
- Install with: `sudo apt install mingw-w64`
- Verify with: `x86_64-w64-mingw32-g++ --version`

**windres command not found**
- MinGW-w64 package includes windres
- Check installation: `which x86_64-w64-mingw32-windres`

**Compilation errors**
- Ensure all source files are present
- Check file permissions on build.sh
- Review error output for specific issues

### Runtime Issues (on Windows)

**Application won't start**
- Ensure you're running on Windows 10 or later
- Try running as administrator (though it shouldn't be required)
- Check Windows Event Viewer for errors

**Icon doesn't appear in system tray**
- Check Windows notification settings
- Restart Windows Explorer: `Ctrl+Shift+Esc` → Windows Explorer → Restart
- Ensure no other instance is already running

**Icon doesn't update when mouse buttons change**
- Try exiting and restarting the application
- Check if another application is interfering

## Known Limitations

- Icon changes require system tray refresh in some cases
- Setting persists until changed again or system restart
- Only affects primary mouse device on multi-mouse systems

## Contributing

Feel free to fork and modify the code for your needs. Suggestions for improvements:
- Keyboard shortcut support
- Notification on orientation change
- Sound feedback option

## Credits

Primary - A utility application for quick mouse button configuration changes.
Built with MinGW-w64 cross-compiler on Linux.
