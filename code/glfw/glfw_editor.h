#ifndef GLFW_EDITOR_H
#define GLFW_EDITOR_H

// TODO(hbr): merge this with [win32_platform_input]
struct linux_platform_input
{
 u32 EventCount;
#define LINUX_MAX_PLATFORM_EVENT_COUNT 128
 platform_event Events[LINUX_MAX_PLATFORM_EVENT_COUNT];
};

#endif //GLFW_EDITOR_H