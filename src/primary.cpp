#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <shellapi.h>
#include "../resources/resource.h"
#include "../resources/app_strings.h"

// Global variables
const wchar_t* CLASS_NAME = APP_WINDOW_CLASS;
const wchar_t* REGISTRY_KEY = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
const wchar_t* REGISTRY_VALUE = APP_REGISTRY_VALUE;
const wchar_t* SETTINGS_REGISTRY_KEY = APP_SETTINGS_REGISTRY_KEY;
const wchar_t* AUTOSWITCH_VALUE = L"AutoSwitch";
const wchar_t* BASE_MOUSE_COUNT_VALUE = L"BaseMouseCount";
NOTIFYICONDATA g_nid = {};
HWND g_hwndMain = NULL;
bool g_lastDisplayState = false;  // Track last external mouse connection state

// Forward declarations
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK OptionsDialogProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK AboutDialogProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
bool GetCurrentMouseState();
void FlipMouseOrientation();
UINT GetIconForCurrentState();
void AddTrayIcon(HWND hwnd, UINT iconID);
void UpdateTrayIcon(HWND hwnd, UINT iconID);
void RemoveTrayIcon(HWND hwnd);
void ShowContextMenu(HWND hwnd, POINT pt);
void UpdateMenuChecks(HMENU hMenu);
void ShowAboutDialog(HWND hwnd);
void ShowOptionsDialog(HWND hwnd);
bool IsStartupEnabled();
bool SetStartupEnabled(bool enable);
bool IsAutoSwitchEnabled();
bool SetAutoSwitchEnabled(bool enable);
int GetCurrentMouseDeviceCount();
int GetBaseMouseCount();
bool SetBaseMouseCount(int count);
bool IsExternalMouseConnected();
void CheckAndApplyAutoSwitch();
void StartAutoSwitchMonitoring(HWND hwnd);
void StopAutoSwitchMonitoring(HWND hwnd);
wchar_t* GetExecutablePath();

// Entry point
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    // Register window class
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON_APP));
    wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON_APP));

    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, L"Window registration failed!", APP_NAME, MB_ICONERROR | MB_OK);
        return 1;
    }

    // Create hidden window for message processing
    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        APP_NAME,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL) {
        MessageBox(NULL, L"Window creation failed!", APP_NAME, MB_ICONERROR | MB_OK);
        return 1;
    }

    // Store window handle globally
    g_hwndMain = hwnd;

    // Message loop
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

// Window procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            // Initialize tray icon with current system state
            AddTrayIcon(hwnd, GetIconForCurrentState());
            // Start auto-switch monitoring if enabled
            if (IsAutoSwitchEnabled()) {
                // Initialize to opposite state to force initial application
                g_lastDisplayState = !IsExternalMouseConnected();
                StartAutoSwitchMonitoring(hwnd);
                CheckAndApplyAutoSwitch();  // Apply immediately (will detect change and apply)
            }
            return 0;

        case WM_TIMER:
            if (wParam == TIMER_AUTOSWITCH) {
                CheckAndApplyAutoSwitch();
            }
            return 0;

        case WM_TRAYICON:
            switch (LOWORD(lParam)) {
                case WM_LBUTTONDBLCLK:
                case WM_RBUTTONDBLCLK:
                    // Double-click (either button): flip mouse orientation
                    FlipMouseOrientation();
                    UpdateTrayIcon(hwnd, GetIconForCurrentState());
                    break;

                case WM_RBUTTONUP:
                    // Right single-click: show context menu
                    POINT pt;
                    GetCursorPos(&pt);
                    ShowContextMenu(hwnd, pt);
                    break;
            }
            return 0;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDM_RIGHTHANDED:
                    SwapMouseButton(FALSE);
                    UpdateTrayIcon(hwnd, GetIconForCurrentState());
                    break;

                case IDM_LEFTHANDED:
                    SwapMouseButton(TRUE);
                    UpdateTrayIcon(hwnd, GetIconForCurrentState());
                    break;

                case IDM_OPTIONS:
                    ShowOptionsDialog(hwnd);
                    break;

                case IDM_ABOUT:
                    ShowAboutDialog(hwnd);
                    break;

                case IDM_EXIT:
                    RemoveTrayIcon(hwnd);
                    PostQuitMessage(0);
                    break;
            }
            return 0;

        case WM_DESTROY:
            StopAutoSwitchMonitoring(hwnd);
            RemoveTrayIcon(hwnd);
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// Get current mouse button configuration
// Returns true if buttons are swapped (left-handed), false if normal (right-handed)
bool GetCurrentMouseState() {
    return GetSystemMetrics(SM_SWAPBUTTON) != 0;
}

// Flip mouse button orientation
void FlipMouseOrientation() {
    bool currentState = GetCurrentMouseState();
    SwapMouseButton(!currentState);
}

// Get icon resource ID based on current system state
UINT GetIconForCurrentState() {
    return GetCurrentMouseState() ? IDI_ICON_LEFT : IDI_ICON_RIGHT;
}

// Add tray icon
void AddTrayIcon(HWND hwnd, UINT iconID) {
    g_nid.cbSize = sizeof(NOTIFYICONDATA);
    g_nid.hWnd = hwnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(iconID));
    wcsncpy(g_nid.szTip, APP_TRAY_TOOLTIP,
            sizeof(g_nid.szTip) / sizeof(wchar_t) - 1);
    g_nid.szTip[sizeof(g_nid.szTip) / sizeof(wchar_t) - 1] = L'\0';

    Shell_NotifyIcon(NIM_ADD, &g_nid);
}

// Update tray icon
void UpdateTrayIcon(HWND hwnd, UINT iconID) {
    g_nid.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(iconID));
    Shell_NotifyIcon(NIM_MODIFY, &g_nid);
}

// Remove tray icon
void RemoveTrayIcon(HWND hwnd) {
    Shell_NotifyIcon(NIM_DELETE, &g_nid);
}

// Show context menu
void ShowContextMenu(HWND hwnd, POINT pt) {
    HMENU hMenu = CreatePopupMenu();
    if (hMenu) {
        AppendMenu(hMenu, MF_STRING, IDM_RIGHTHANDED, L"Right-handed");
        AppendMenu(hMenu, MF_STRING, IDM_LEFTHANDED, L"Left-handed");
        AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
        AppendMenu(hMenu, MF_STRING, IDM_OPTIONS, L"Options...");
        AppendMenu(hMenu, MF_STRING, IDM_ABOUT, L"About");
        AppendMenu(hMenu, MF_STRING, IDM_EXIT, L"Exit");

        UpdateMenuChecks(hMenu);

        // Required for proper menu behavior
        SetForegroundWindow(hwnd);

        TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);

        DestroyMenu(hMenu);
    }
}

// Update menu checkmarks based on current state
void UpdateMenuChecks(HMENU hMenu) {
    bool isLeftHanded = GetCurrentMouseState();

    CheckMenuItem(hMenu, IDM_RIGHTHANDED, MF_BYCOMMAND | (isLeftHanded ? MF_UNCHECKED : MF_CHECKED));
    CheckMenuItem(hMenu, IDM_LEFTHANDED, MF_BYCOMMAND | (isLeftHanded ? MF_CHECKED : MF_UNCHECKED));
}

// About dialog procedure
INT_PTR CALLBACK AboutDialogProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG: {
            // Set the about text
            wchar_t message[256];
            wsprintf(message,
                     L"%s v%s\n\n"
                     L"Quickly toggle mouse button configuration\n"
                     L"between right-handed and left-handed modes.\n\n"
                     L"Double-click the tray icon with either button to flip.\n"
                     L"Right-click for menu.",
                     APP_NAME, APP_VERSION);
            SetDlgItemText(hwndDlg, IDC_ABOUT_TEXT, message);
            return TRUE;
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
                EndDialog(hwndDlg, LOWORD(wParam));
                return TRUE;
            }
            break;
    }

    return FALSE;
}

// Show about dialog
void ShowAboutDialog(HWND hwnd) {
    DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ABOUT), hwnd, AboutDialogProc);
}

// Get full path to the executable
wchar_t* GetExecutablePath() {
    static wchar_t path[MAX_PATH];
    GetModuleFileName(NULL, path, MAX_PATH);
    return path;
}

// Check if startup is enabled
bool IsStartupEnabled() {
    HKEY hKey;
    bool enabled = false;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, REGISTRY_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        wchar_t value[MAX_PATH];
        DWORD size = sizeof(value);
        DWORD type;

        if (RegQueryValueEx(hKey, REGISTRY_VALUE, NULL, &type, (LPBYTE)value, &size) == ERROR_SUCCESS) {
            if (type == REG_SZ) {
                enabled = true;
            }
        }

        RegCloseKey(hKey);
    }

    return enabled;
}

// Enable or disable startup with Windows
bool SetStartupEnabled(bool enable) {
    HKEY hKey;
    bool success = false;
    LONG result;

    result = RegOpenKeyEx(HKEY_CURRENT_USER, REGISTRY_KEY, 0, KEY_WRITE, &hKey);

    if (enable) {
        // Add to startup - key must exist
        if (result == ERROR_SUCCESS) {
            wchar_t* path = GetExecutablePath();
            DWORD size = (wcslen(path) + 1) * sizeof(wchar_t);

            if (RegSetValueEx(hKey, REGISTRY_VALUE, 0, REG_SZ, (LPBYTE)path, size) == ERROR_SUCCESS) {
                success = true;
            }
            RegCloseKey(hKey);
        }
    } else {
        // Remove from startup
        if (result == ERROR_SUCCESS) {
            // Key exists, try to delete the value
            LONG deleteResult = RegDeleteValue(hKey, REGISTRY_VALUE);
            if (deleteResult == ERROR_SUCCESS || deleteResult == ERROR_FILE_NOT_FOUND) {
                success = true;
            }
            RegCloseKey(hKey);
        } else if (result == ERROR_FILE_NOT_FOUND) {
            // Key doesn't exist, so value doesn't exist either - this is success for deletion
            success = true;
        }
    }

    return success;
}

// Options dialog procedure
INT_PTR CALLBACK OptionsDialogProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG: {
            // Set checkbox states based on current settings
            CheckDlgButton(hwndDlg, IDC_STARTUP_CHECKBOX,
                          IsStartupEnabled() ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwndDlg, IDC_AUTOSWITCH_CHECKBOX,
                          IsAutoSwitchEnabled() ? BST_CHECKED : BST_UNCHECKED);

            // Display current detected mouse device count
            int detectedCount = GetCurrentMouseDeviceCount();
            wchar_t countStr[16];
            wsprintf(countStr, L"%d", detectedCount);
            SetDlgItemText(hwndDlg, IDC_DETECTED_DEVICES_LABEL, countStr);

            // Set base mouse count in edit control
            int baseCount = GetBaseMouseCount();
            wsprintf(countStr, L"%d", baseCount);
            SetDlgItemText(hwndDlg, IDC_BASE_DEVICES_EDIT, countStr);

            return TRUE;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK: {
                    // Get checkbox states
                    bool startupEnabled = (IsDlgButtonChecked(hwndDlg, IDC_STARTUP_CHECKBOX) == BST_CHECKED);
                    bool autoSwitchEnabled = (IsDlgButtonChecked(hwndDlg, IDC_AUTOSWITCH_CHECKBOX) == BST_CHECKED);

                    // Get base mouse count from edit control
                    wchar_t countStr[16];
                    GetDlgItemText(hwndDlg, IDC_BASE_DEVICES_EDIT, countStr, 16);
                    int baseCount = _wtoi(countStr);

                    // Validate base count
                    if (baseCount < 1) {
                        MessageBox(hwndDlg,
                                  L"Base device count must be at least 1.",
                                  L"Invalid Input",
                                  MB_ICONWARNING | MB_OK);
                        return TRUE;
                    }

                    // Apply startup setting
                    if (!SetStartupEnabled(startupEnabled)) {
                        MessageBox(hwndDlg,
                                  L"Failed to update startup settings. Please check your permissions.",
                                  L"Error",
                                  MB_ICONERROR | MB_OK);
                    }

                    // Apply auto-switch setting
                    if (!SetAutoSwitchEnabled(autoSwitchEnabled)) {
                        MessageBox(hwndDlg,
                                  L"Failed to update auto-switch settings. Please check your permissions.",
                                  L"Error",
                                  MB_ICONERROR | MB_OK);
                    } else {
                        // Start or stop monitoring based on setting
                        if (autoSwitchEnabled) {
                            StartAutoSwitchMonitoring(g_hwndMain);
                            CheckAndApplyAutoSwitch();  // Apply immediately (will detect change and apply)
                        } else {
                            StopAutoSwitchMonitoring(g_hwndMain);
                        }
                    }

                    // Apply base mouse count setting
                    if (!SetBaseMouseCount(baseCount)) {
                        MessageBox(hwndDlg,
                                  L"Failed to update base mouse count. Please check your permissions.",
                                  L"Error",
                                  MB_ICONERROR | MB_OK);
                    } else {
                        // Re-check if auto-switch is enabled, to apply new settings immediately
                        if (autoSwitchEnabled) {
                            CheckAndApplyAutoSwitch();
                        }
                    }

                    EndDialog(hwndDlg, IDOK);
                    return TRUE;
                }

                case IDCANCEL:
                    EndDialog(hwndDlg, IDCANCEL);
                    return TRUE;
            }
            break;
    }

    return FALSE;
}

// Show options dialog
void ShowOptionsDialog(HWND hwnd) {
    DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_OPTIONS), hwnd, OptionsDialogProc);
}

// Check if auto-switch is enabled (default: true)
bool IsAutoSwitchEnabled() {
    HKEY hKey;
    bool enabled = true;  // Default to enabled if setting doesn't exist

    if (RegOpenKeyEx(HKEY_CURRENT_USER, SETTINGS_REGISTRY_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD value = 1;  // Default to 1 (enabled)
        DWORD size = sizeof(value);
        DWORD type;

        if (RegQueryValueEx(hKey, AUTOSWITCH_VALUE, NULL, &type, (LPBYTE)&value, &size) == ERROR_SUCCESS) {
            if (type == REG_DWORD) {
                enabled = (value != 0);
            }
        }
        // If value doesn't exist, keep default (true)

        RegCloseKey(hKey);
    }
    // If registry key doesn't exist at all, keep default (true)

    return enabled;
}

// Enable or disable auto-switch
bool SetAutoSwitchEnabled(bool enable) {
    HKEY hKey;
    bool success = false;
    DWORD disposition;

    // Create or open the settings key
    if (RegCreateKeyEx(HKEY_CURRENT_USER, SETTINGS_REGISTRY_KEY, 0, NULL, 0,
                       KEY_WRITE, NULL, &hKey, &disposition) == ERROR_SUCCESS) {
        // Always write the value (1 for enabled, 0 for disabled)
        DWORD value = enable ? 1 : 0;
        if (RegSetValueEx(hKey, AUTOSWITCH_VALUE, 0, REG_DWORD, (LPBYTE)&value, sizeof(value)) == ERROR_SUCCESS) {
            success = true;
        }

        RegCloseKey(hKey);
    }

    return success;
}

// Get base mouse device count (default: 1)
int GetBaseMouseCount() {
    HKEY hKey;
    int count = 1;  // Default to 1 if setting doesn't exist

    if (RegOpenKeyEx(HKEY_CURRENT_USER, SETTINGS_REGISTRY_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD value = 1;  // Default to 1
        DWORD size = sizeof(value);
        DWORD type;

        if (RegQueryValueEx(hKey, BASE_MOUSE_COUNT_VALUE, NULL, &type, (LPBYTE)&value, &size) == ERROR_SUCCESS) {
            if (type == REG_DWORD && value > 0) {
                count = (int)value;
            }
        }

        RegCloseKey(hKey);
    }

    return count;
}

// Set base mouse device count
bool SetBaseMouseCount(int count) {
    if (count < 1) {
        return false;  // Invalid count
    }

    HKEY hKey;
    bool success = false;
    DWORD disposition;

    // Create or open the settings key
    if (RegCreateKeyEx(HKEY_CURRENT_USER, SETTINGS_REGISTRY_KEY, 0, NULL, 0,
                       KEY_WRITE, NULL, &hKey, &disposition) == ERROR_SUCCESS) {
        DWORD value = (DWORD)count;
        if (RegSetValueEx(hKey, BASE_MOUSE_COUNT_VALUE, 0, REG_DWORD, (LPBYTE)&value, sizeof(value)) == ERROR_SUCCESS) {
            success = true;
        }

        RegCloseKey(hKey);
    }

    return success;
}

// Get the current number of mouse devices detected
int GetCurrentMouseDeviceCount() {
    // Get the number of raw input devices
    UINT numDevices = 0;
    if (GetRawInputDeviceList(NULL, &numDevices, sizeof(RAWINPUTDEVICELIST)) != 0) {
        return 0;  // Error getting device count
    }

    if (numDevices == 0) {
        return 0;  // No devices
    }

    // Allocate buffer for device list
    RAWINPUTDEVICELIST* deviceList = new RAWINPUTDEVICELIST[numDevices];
    if (GetRawInputDeviceList(deviceList, &numDevices, sizeof(RAWINPUTDEVICELIST)) == (UINT)-1) {
        delete[] deviceList;
        return 0;  // Error getting device list
    }

    // Count mouse devices
    int mouseCount = 0;
    for (UINT i = 0; i < numDevices; i++) {
        if (deviceList[i].dwType == RIM_TYPEMOUSE) {
            mouseCount++;
        }
    }

    delete[] deviceList;
    return mouseCount;
}

// Check if an external mouse is connected
bool IsExternalMouseConnected() {
    int currentCount = GetCurrentMouseDeviceCount();
    int baseCount = GetBaseMouseCount();

    // If current count exceeds base count, we have external mouse(s)
    return (currentCount > baseCount);
}

// Check if external mouse is connected and apply appropriate mouse configuration
void CheckAndApplyAutoSwitch() {
    bool externalMouseConnected = IsExternalMouseConnected();

    // Only switch if the state has changed to avoid unnecessary operations
    if (externalMouseConnected != g_lastDisplayState) {
        g_lastDisplayState = externalMouseConnected;

        if (externalMouseConnected) {
            // External mouse connected → Left-handed
            SwapMouseButton(TRUE);
        } else {
            // Only trackpad (no external mouse) → Right-handed
            SwapMouseButton(FALSE);
        }

        // Update tray icon to reflect new state
        if (g_hwndMain) {
            UpdateTrayIcon(g_hwndMain, GetIconForCurrentState());
        }
    }
}

// Start auto-switch monitoring
// Note: g_lastDisplayState should be initialized by caller before calling this
void StartAutoSwitchMonitoring(HWND hwnd) {
    // Set timer to check every 2 seconds
    SetTimer(hwnd, TIMER_AUTOSWITCH, 2000, NULL);
}

// Stop auto-switch monitoring
void StopAutoSwitchMonitoring(HWND hwnd) {
    KillTimer(hwnd, TIMER_AUTOSWITCH);
}
