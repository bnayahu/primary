#ifndef APP_STRINGS_H
#define APP_STRINGS_H

// String building macros
#define STRINGIFY(x) #x
#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)

// Base application name (single source of truth)
#define APP_NAME_BASE MouseFlip

// Application name in different formats
#define APP_NAME_A                      STRINGIFY(APP_NAME_BASE)  // ANSI version
#define APP_NAME                        WIDEN(APP_NAME_A)         // Wide string version
#define APP_VERSION                     L"1.0"
#define APP_WINDOW_CLASS                APP_NAME L"WindowClass"

// Registry keys and values
#define APP_REGISTRY_VALUE              APP_NAME
#define APP_SETTINGS_REGISTRY_KEY       L"Software\\" APP_NAME

// UI Strings
#define APP_TRAY_TOOLTIP                APP_NAME L" - Double-click either button to flip"
#define APP_OPTIONS_DIALOG_CAPTION      APP_NAME_A " Options"
#define APP_ABOUT_DIALOG_CAPTION        "About " APP_NAME_A
#define APP_STARTUP_CHECKBOX_TEXT       "Start " APP_NAME_A " when Windows starts"

#endif // APP_STRINGS_H
