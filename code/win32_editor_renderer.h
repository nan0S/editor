#ifndef WIN32_EDITOR_RENDERER_H
#define WIN32_EDITOR_RENDERER_H

#define WIN32_RENDERER_INIT(Name) platform_renderer *Name(arena *Arena, platform_renderer_limits *Limits, platform_api PlatformAPI, imgui_bindings ImGuiBindings, HWND Window, HDC WindowDC)
typedef WIN32_RENDERER_INIT(win32_renderer_init);

#endif //WIN32_EDITOR_RENDERER_H
