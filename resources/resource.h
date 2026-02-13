#ifndef RESOURCE_H
#define RESOURCE_H

// Icon resource IDs
#define IDI_ICON_RIGHT              101
#define IDI_ICON_LEFT               102
#define IDI_ICON_APP                103

// Dialog IDs
#define IDD_OPTIONS                 201
#define IDD_ABOUT                   202

// Control IDs
#define IDC_STARTUP_CHECKBOX        2001
#define IDC_AUTOSWITCH_CHECKBOX     2002
#define IDC_ABOUT_ICON              2003
#define IDC_ABOUT_TEXT              2004

// Menu item IDs
#define IDM_RIGHTHANDED             1001
#define IDM_LEFTHANDED              1002
#define IDM_OPTIONS                 1003
#define IDM_ABOUT                   1004
#define IDM_EXIT                    1005

// Custom window message for tray icon events
#define WM_TRAYICON                 (WM_USER + 1)

// Timer IDs
#define TIMER_AUTOSWITCH            1

#endif // RESOURCE_H
