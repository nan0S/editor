#ifndef WIN32_EDITOR_RENDERER_H
#define WIN32_EDITOR_RENDERER_H

#define WIN32_RENDERER_INIT(Name) renderer *Name(arena *Arena, renderer_memory *Memory, HWND Window, HDC WindowDC)
typedef WIN32_RENDERER_INIT(win32_renderer_init);

union win32_renderer_function_table
{
 struct {
  win32_renderer_init *Init;
  renderer_begin_frame *BeginFrame;
  renderer_end_frame *EndFrame;
  renderer_on_code_reload *OnCodeReload;
 };
 void *Functions[4];
};
global char const *Win32RendererFunctionTableNames[] = {
 "Win32RendererInit",
 "Win32RendererBeginFrame",
 "Win32RendererEndFrame",
 "Win32RendererOnCodeReload",
};
StaticAssert(SizeOf(MemberOf(win32_renderer_function_table, Functions)) ==
             SizeOf(win32_renderer_function_table), 
             Win32RendererFunctionTableLayoutIsCorrect);
StaticAssert(ArrayCount(MemberOf(win32_renderer_function_table, Functions)) ==
             ArrayCount(Win32RendererFunctionTableNames),
             Win32RendererFunctionTableNamesDefined);

#endif //WIN32_EDITOR_RENDERER_H
