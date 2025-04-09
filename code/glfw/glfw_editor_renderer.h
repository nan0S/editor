#ifndef GLFW_EDITOR_RENDERER_H
#define GLFW_EDITOR_RENDERER_H

struct GLFWwindow;

#define GLFW_RENDERER_INIT(Name) renderer *Name(arena *Arena, renderer_memory *Memory, GLFWwindow *Window)
typedef GLFW_RENDERER_INIT(glfw_renderer_init);

union glfw_renderer_function_table
{
 struct {
  glfw_renderer_init *Init;
  renderer_begin_frame *BeginFrame;
  renderer_end_frame *EndFrame;
  renderer_on_code_reload *OnCodeReload;
 };
 void *Functions[4];
};
global char const *GLFWRendererFunctionTableNames[] = {
 "GLFWRendererInit",
 "GLFWRendererBeginFrame",
 "GLFWRendererEndFrame",
 "GLFWRendererOnCodeReload",
};
StaticAssert(SizeOf(MemberOf(glfw_renderer_function_table, Functions)) ==
             SizeOf(glfw_renderer_function_table), 
             GLFWRendererFunctionTableLayoutIsCorrect);
StaticAssert(ArrayCount(MemberOf(glfw_renderer_function_table, Functions)) ==
             ArrayCount(GLFWRendererFunctionTableNames),
             GLFWRendererFunctionTableNamesDefined);

#endif //GLFW_EDITOR_RENDERER_H
