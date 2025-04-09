#ifndef GLFW_EDITOR_H
#define GLFW_EDITOR_H

struct glfw_platform_input
{
 u32 EventCount;
#define GLFW_MAX_EVENT_COUNT 128
 platform_event Events[GLFW_MAX_EVENT_COUNT];
};

struct glfw_state
{
 b32 Running;
 glfw_platform_input GLFWInput;
 platform_key GLFWToPlatformKeyTable[GLFW_KEY_LAST + 1];
 arena *InputArena;
};

#endif //GLFW_EDITOR_H
