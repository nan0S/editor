#ifndef WIN32_EDITOR_H
#define WIN32_EDITOR_H

struct win32_platform_input
{
 u32 EventCount;
#define WIN32_MAX_EVENT_COUNT 128
 platform_event Events[WIN32_MAX_EVENT_COUNT];
};

struct win32_hot_reload_task
{
 hot_reload_library *Code;
 b32 CodeReloaded;
};

#endif //WIN32_EDITOR_H
