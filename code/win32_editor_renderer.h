#ifndef WIN32_EDITOR_RENDERER_H
#define WIN32_EDITOR_RENDERER_H

#define WIN32_RENDERER_INIT() platform_renderer *Win32RendererInit(arena *Arena, platform_renderer_limits *Limits, HWND Window)
#define WIN32_RENDERER_BEGIN_FRAME() RENDERER_BEGIN_FRAME(Win32BeginFrame)
#define WIN32_RENDERER_END_FRAME() RENDERER_END_FRAME(Win32EndFrame)

#endif //WIN32_EDITOR_RENDERER_H
