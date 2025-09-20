/* ========================================================================
   Parametric Curve Editor
   Master's Thesis by Hubert Obrzut
   Supervisor: Paweł Woźny
   University of Wrocław
   Faculty of Mathematics and Computer Science
   Institute of Computer Science
   Date: September 2025
   ======================================================================== */

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
 GLFWwindow *Window;
 b32 FullScreen;
};

struct glfw_imgui_maybe_capture_input_result
{
 b32 ImGuiWantCaptureMouse;
 b32 ImGuiWantCaptureKeyboard;
};

#endif //GLFW_EDITOR_H
