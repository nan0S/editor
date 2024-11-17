#ifndef WIN32_EDITOR_RENDERER_H
#define WIN32_EDITOR_RENDERER_H

#define WIN32_RENDERER_INIT(Name) platform_renderer *Name(arena *Arena, platform_renderer_limits *Limits, platform_api PlatformAPI, HWND Window, HDC WindowDC)
typedef WIN32_RENDERER_INIT(win32_renderer_init);

union win32_renderer_function_table
{
 struct {
  win32_renderer_init *Init;
  renderer_begin_frame *BeginFrame;
  renderer_end_frame *EndFrame;
 };
 void *Functions[3];
};
global char const *Win32RendererFunctionTableNames[] = {
 "Win32RendererInit",
 "Win32RendererBeginFrame",
 "Win32RendererEndFrame",
};
StaticAssert(SizeOf(win32_renderer_function_table) == SizeOf(win32_renderer_function_table::Functions), Win32RendererFunctionTableLayoutIsCorrect);
StaticAssert(ArrayCount(win32_renderer_function_table::Functions) == ArrayCount(Win32RendererFunctionTableNames), Win32RendererFunctionTableNamesDefined);

#endif //WIN32_EDITOR_RENDERER_H
