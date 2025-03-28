#ifndef WIN32_EDITOR_H
#define WIN32_EDITOR_H

struct win32_platform_input
{
 u32 EventCount;
#define WIN32_MAX_EVENT_COUNT 128
 platform_event Events[WIN32_MAX_EVENT_COUNT];
};

struct win32_state
{
 win32_platform_input Win32Input;
 arena *InputArena;
#define WIN32_KEY_COUNT 0xFF
 platform_key Win32ToPlatformKeyTable[WIN32_KEY_COUNT];
};

#endif //WIN32_EDITOR_H
