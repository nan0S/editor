#include "editor_renderer_opengl.h"
#include "editor_renderer_opengl.cpp"

internal renderer *
GLFWRendererInit(arena *Arena, renderer_memory *Memory, GLFWwindow *Window)
{
 ProfileFunctionBegin();
 
 opengl *OpenGL = PushStruct(Arena, opengl);
 glfw_opengl_renderer *GLFWRenderer = PushStruct(Arena, glfw_opengl_renderer);
 GLFWRenderer->Window = Window;
 OpenGL->Header.Platform = GLFWRenderer;
 
 glfwMakeContextCurrent(Window);
 if (gl3wInit() == 0)
 {
#if BUILD_DEV
  glfwSwapInterval(0);
#else
  glfwSwapInterval(1);
#endif
  
#define OpenGLFunction(Name) OpenGL->Name = Name
  OpenGLFunction(glGenVertexArrays);
  OpenGLFunction(glBindVertexArray);
  OpenGLFunction(glEnableVertexAttribArray);
  OpenGLFunction(glDisableVertexAttribArray);
  OpenGLFunction(glVertexAttribPointer);
  OpenGLFunction(glGetAttribLocation);
  OpenGLFunction(glActiveTexture);
  OpenGLFunction(glGenBuffers);
  OpenGLFunction(glBindBuffer);
  OpenGLFunction(glBufferData);
  OpenGLFunction(glDrawBuffers);
  OpenGLFunction(glCreateProgram);
  OpenGLFunction(glCreateShader);
  OpenGLFunction(glAttachShader);
  OpenGLFunction(glCompileShader);
  OpenGLFunction(glShaderSource);
  OpenGLFunction(glLinkProgram);
  OpenGLFunction(glGetProgramInfoLog);
  OpenGLFunction(glGetShaderInfoLog);
  OpenGLFunction(glUseProgram);
  OpenGLFunction(glValidateProgram);
  OpenGLFunction(glGetUniformLocation);
  OpenGLFunction(glUniform1i);
  OpenGLFunction(glUniform1f);
  OpenGLFunction(glUniform2fv);
  OpenGLFunction(glUniform3fv);
  OpenGLFunction(glUniform4fv);
  OpenGLFunction(glUniformMatrix3fv);
  OpenGLFunction(glUniformMatrix4fv);
  OpenGLFunction(glGetProgramiv);
  OpenGLFunction(glDeleteProgram);
  OpenGLFunction(glDeleteShader);
  OpenGLFunction(glDrawArrays);
  OpenGLFunction(glGenerateMipmap);
  OpenGLFunction(glDebugMessageCallback);
  OpenGLFunction(glVertexAttribDivisor);
  OpenGLFunction(glDrawArraysInstanced);
  OpenGLFunction(glDrawArraysInstancedBaseInstance);
#undef OpenGLFunction
  
  OpenGLInit(OpenGL, Arena, Memory);
 }
 
 ProfileEnd();
 
 return Cast(renderer *)OpenGL;
}

RENDERER_BEGIN_FRAME(GLFWRendererBeginFrame)
{
 opengl *OpenGL = Cast(opengl *)Renderer;
 render_frame *Frame = OpenGLBeginFrame(OpenGL, Memory, WindowDim);
 return Frame;
}

RENDERER_END_FRAME(GLFWRendererEndFrame)
{
 ProfileFunctionBegin();
 
 opengl *OpenGL = Cast(opengl *)Renderer;
 glfw_opengl_renderer *GLFW = Cast(glfw_opengl_renderer *)Renderer->Header.Platform;
 OpenGLEndFrame(OpenGL, Memory, Frame);
 
 ProfileBlock("glfwSwapBuffers")
 {
  glfwSwapBuffers(GLFW->Window);
 }
 
 ProfileEnd();
}
