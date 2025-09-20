/* ========================================================================
   Parametric Curve Editor
   Master's Thesis by Hubert Obrzut
   Supervisor: Paweł Woźny
   University of Wrocław
   Faculty of Mathematics and Computer Science
   Institute of Computer Science
   Date: September 2025
   ======================================================================== */

#ifndef X11_EDITOR_RENDERER_H
#define X11_EDITOR_RENDERER_H

struct x11_renderer_init_result
{
 renderer *Renderer;
 Window X11Window;
};

#define X11_CREATE_WINDOW_AND_INIT_RENDERER(Name) x11_renderer_init_result Name(arena *Arena, Display *Display, u32 PosX, u32 PosY, u32 WindowWidth, u32 WindowHeight, u32 WindowBorderWidth);
typedef X11_CREATE_WINDOW_AND_INIT_RENDERER(x11_create_window_and_init_renderer);

union x11_renderer_function_table
{
 struct {
  x11_create_window_and_init_renderer *CreateWindowAndInitRenderer;
  renderer_begin_frame *BeginFrame;
  renderer_end_frame *EndFrame;
 };
 void *Functions[3];
};
global char const *X11RendererFunctionTableNames[] = {
 "X11CreateWindowAndInitRenderer",
 "X11RendererBeginFrame",
 "X11RendererEndFrame",
};
StaticAssert(SizeOf(x11_renderer_function_table) == SizeOf(x11_renderer_function_table::Functions), X11RendererFunctionTableLayoutIsCorrect);
StaticAssert(ArrayCount(x11_renderer_function_table::Functions) == ArrayCount(X11RendererFunctionTableNames), X11RendererFunctionTableNamesDefined);

#endif //X11_EDITOR_RENDERER_H