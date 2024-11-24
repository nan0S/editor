#include <windows.h>
#include <gl/gl.h>

#include "base_ctx_crack.h"
#include "base_core.h"
#include "base_string.h"
#include "base_os.h"

#include "editor_memory.h"
#include "editor_math.h"
#include "editor_imgui_bindings.h"
#include "editor_platform.h"
#include "editor_renderer.h"

#include "win32_editor_renderer.h"
#include "win32_editor_renderer_opengl.h"
#include "win32_editor_imgui_bindings.h"
#include "win32_shared.h"

#include "base_core.cpp"
#include "base_string.cpp"
#include "base_os.cpp"

#include "editor_memory.cpp"
#include "editor_math.cpp"

#include "editor_third_party_inc.h"

platform_api Platform;

internal void
Win32SetPixelFormat(opengl *OpenGL, HDC WindowDC)
{
 WIN32_BEGIN_DEBUG_BLOCK(SetPixelFormat);
 
 int SuggestedPixelFormatIndex = 0;
 UINT ExtendedPicked = 0;
 
 if (OpenGL->wglChoosePixelFormatARB)
 {
  int AttribIList[] =
  {
   WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
   WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
   WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
   WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
   WGL_COLOR_BITS_ARB, 32,
   WGL_ALPHA_BITS_ARB, 8,
   WGL_DEPTH_BITS_ARB, 24,
   // NOTE(hbr): When I set WGL_SAMPLES_ARB to some values (e.g. 16), ImGui text becomes blurry.
   // I don't know why that is.
   WGL_SAMPLES_ARB, 8,
   // NOTE(hbr): I don't know if this is really needed, but Casey uses it
   WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
   0
  };
  
  OpenGL->wglChoosePixelFormatARB(WindowDC,
                                  AttribIList,
                                  0,
                                  1,
                                  &SuggestedPixelFormatIndex,
                                  &ExtendedPicked);
 }
 
 if (!ExtendedPicked)
 {
  PIXELFORMATDESCRIPTOR DesiredPixelFormat = {};
  DesiredPixelFormat.nSize = SizeOf(DesiredPixelFormat);
  DesiredPixelFormat.nVersion = 1;
  DesiredPixelFormat.iPixelType = PFD_TYPE_RGBA;
  DesiredPixelFormat.dwFlags = (PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER);
  DesiredPixelFormat.cColorBits = 32;
  DesiredPixelFormat.cAlphaBits = 8;
  DesiredPixelFormat.cDepthBits = 24;
  DesiredPixelFormat.iLayerType = PFD_MAIN_PLANE;
  
  SuggestedPixelFormatIndex = ChoosePixelFormat(WindowDC, &DesiredPixelFormat);
 }
 
 PIXELFORMATDESCRIPTOR SuggestedPixelFormat;
 DescribePixelFormat(WindowDC, SuggestedPixelFormatIndex, SizeOf(SuggestedPixelFormat), &SuggestedPixelFormat);
 SetPixelFormat(WindowDC, SuggestedPixelFormatIndex, &SuggestedPixelFormat);
 
 WIN32_END_DEBUG_BLOCK(SetPixelFormat);
 
}

#define Win32OpenGLFunction(Name) OpenGL->Name = Cast(func_##Name *)wglGetProcAddress(#Name);
DLL_EXPORT
WIN32_RENDERER_INIT(Win32RendererInit)
{
 Platform = Memory->PlatformAPI;
 
 opengl *OpenGL = PushStruct(Arena, opengl);
 OpenGL->Window = Window;
 OpenGL->WindowDC = WindowDC;
 
 //- create false context to retrieve extensions pointers
 {
  WNDCLASSA WindowClass = {};
  { 
   WindowClass.lpfnWndProc = DefWindowProcA;
   WindowClass.hInstance = GetModuleHandle(0);
   WindowClass.lpszClassName = "Win32EditorWGLLoader";
  }
  
  if (RegisterClassA(&WindowClass))
  {
   HWND FalseWindow =
    CreateWindowExA(0,
                    WindowClass.lpszClassName,
                    "Parametric Curves Editor",
                    0,
                    CW_USEDEFAULT,
                    CW_USEDEFAULT,
                    CW_USEDEFAULT,
                    CW_USEDEFAULT,
                    0,
                    0,
                    WindowClass.hInstance,
                    0);
   
   HDC FalseWindowDC = GetWindowDC(FalseWindow);
   Win32SetPixelFormat(OpenGL, FalseWindowDC);
   
   HGLRC FalseOpenGLRC = wglCreateContext(FalseWindowDC);
   if (wglMakeCurrent(FalseWindowDC, FalseOpenGLRC))
   {
    Win32OpenGLFunction(wglChoosePixelFormatARB);
   }
   
   wglDeleteContext(FalseOpenGLRC);
   ReleaseDC(FalseWindow, FalseWindowDC);
   DestroyWindow(FalseWindow);
  }
 }
 
 //- create opengl context
 Win32SetPixelFormat(OpenGL, WindowDC);
 HGLRC OpenGLRC = wglCreateContext(WindowDC);
 if (wglMakeCurrent(WindowDC, OpenGLRC))
 {
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
  glEnable(GL_MULTISAMPLE);
  // NOTE(hbr): So that glEnable,glEnd with glBindTexture works. When using shaders it is not needed.
  glEnable(GL_TEXTURE_2D);
  
  Win32OpenGLFunction(wglSwapIntervalEXT);
  Win32OpenGLFunction(glGenVertexArrays);
  Win32OpenGLFunction(glBindVertexArray);
  Win32OpenGLFunction(glEnableVertexAttribArray);
  Win32OpenGLFunction(glDisableVertexAttribArray);
  Win32OpenGLFunction(glVertexAttribPointer);
  Win32OpenGLFunction(glActiveTexture);
  Win32OpenGLFunction(glGenBuffers);
  Win32OpenGLFunction(glBindBuffer);
  Win32OpenGLFunction(glBufferData);
  Win32OpenGLFunction(glDrawBuffers);
  Win32OpenGLFunction(glCreateProgram);
  Win32OpenGLFunction(glCreateShader);
  Win32OpenGLFunction(glAttachShader);
  Win32OpenGLFunction(glCompileShader);
  Win32OpenGLFunction(glShaderSource);
  Win32OpenGLFunction(glLinkProgram);
  Win32OpenGLFunction(glGetProgramInfoLog);
  Win32OpenGLFunction(glGetShaderInfoLog);
  Win32OpenGLFunction(glUseProgram);
  Win32OpenGLFunction(glGetUniformLocation);
  Win32OpenGLFunction(glUniform1f);
  Win32OpenGLFunction(glUniform2fv);
  Win32OpenGLFunction(glUniform3fv);
  Win32OpenGLFunction(glUniform4fv);
  Win32OpenGLFunction(glUniformMatrix4fv);
  Win32OpenGLFunction(glGetAttribLocation);
  Win32OpenGLFunction(glValidateProgram);
  Win32OpenGLFunction(glGetProgramiv);
  Win32OpenGLFunction(glDeleteProgram);
  Win32OpenGLFunction(glDeleteShader);
  Win32OpenGLFunction(glDrawArrays);
  Win32OpenGLFunction(glGenerateMipmap);
  
  if (OpenGL->wglSwapIntervalEXT)
  {
   // NOTE(hbr): disable VSync
   OpenGL->wglSwapIntervalEXT(0);
  }
 }
 
 //- allocate texutre indices
 u32 TextureCount = Memory->Limits.MaxTextureCount;
 GLuint *Textures = PushArray(Arena, TextureCount, GLuint);
 OpenGL->MaxTextureCount = TextureCount;
 OpenGL->Textures = Textures;
 
 glGenTextures(Cast(GLsizei)TextureCount, Textures);
 for (u32 TextureIndex = 0;
      TextureIndex < TextureCount;
      ++TextureIndex)
 {
  glBindTexture(GL_TEXTURE_2D, Textures[TextureIndex]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
 }
 
 return Cast(renderer *)OpenGL;
}
#undef Win32OpenGLFunction

DLL_EXPORT
RENDERER_BEGIN_FRAME(Win32RendererBeginFrame)
{
 Platform = Memory->PlatformAPI;
 
 opengl *OpenGL = Cast(opengl *)Renderer;
 
 render_frame *RenderFrame = &OpenGL->RenderFrame;
 RenderFrame->CommandCount = 0;
 RenderFrame->Commands = Memory->CommandBuffer;
 RenderFrame->MaxCommandCount = Memory->MaxCommandCount;
 RenderFrame->WindowDim = WindowDim;
 
 Memory->ImGuiBindings.NewFrame();
 
 return RenderFrame;
}

internal int RenderCommandCmp(render_command *A, render_command *B) { return Cmp(A->ZOffset, B->ZOffset); }

internal m4x4
M3x3ToM4x4OpenGL(m3x3 M)
{
 // NOTE(hbr): OpenGL matrices are column-major
 M = Transpose3x3(M);
 m4x4 R = {
  { {M.M[0][0], M.M[0][1], 0, M.M[0][2]},
   {M.M[1][0], M.M[1][1], 0, M.M[1][2]},
   {        0,         0, 1,         0},
   {M.M[2][0], M.M[2][1], 0, M.M[2][2]}}
 };
 
 return R;
}

internal opengl_program
Win32OpenGLCreateProgram(opengl *OpenGL, char *VertexCode, char *FragmentCode)
{
 opengl_program Result = {};
 
 GLuint VertexShaderID = OpenGL->glCreateShader(GL_VERTEX_SHADER);
 GLchar *VertexShaderCode[] =
 {
  VertexCode,
 };
 OpenGL->glShaderSource(VertexShaderID, ArrayCount(VertexShaderCode), VertexShaderCode, 0);
 OpenGL->glCompileShader(VertexShaderID);
 
 GLuint FragmentShaderID = OpenGL->glCreateShader(GL_FRAGMENT_SHADER);
 GLchar *FragmentShaderCode[] =
 {
  FragmentCode,
 };
 OpenGL->glShaderSource(FragmentShaderID, ArrayCount(FragmentShaderCode), FragmentShaderCode, 0);
 OpenGL->glCompileShader(FragmentShaderID);
 
 GLuint ProgramID = OpenGL->glCreateProgram();
 OpenGL->glAttachShader(ProgramID, VertexShaderID);
 OpenGL->glAttachShader(ProgramID, FragmentShaderID);
 OpenGL->glLinkProgram(ProgramID);
 
 OpenGL->glValidateProgram(ProgramID);
 GLint Linked = false;
 OpenGL->glGetProgramiv(ProgramID, GL_LINK_STATUS, &Linked);
 if(!Linked)
 {
  GLsizei Ignored;
  char VertexErrors[4096];
  char FragmentErrors[4096];
  char ProgramErrors[4096];
  OpenGL->glGetShaderInfoLog(VertexShaderID, sizeof(VertexErrors), &Ignored, VertexErrors);
  OpenGL->glGetShaderInfoLog(FragmentShaderID, sizeof(FragmentErrors), &Ignored, FragmentErrors);
  OpenGL->glGetProgramInfoLog(ProgramID, sizeof(ProgramErrors), &Ignored, ProgramErrors);
  
  Assert(!"Shader validation failed");
 }
 
 OpenGL->glDeleteShader(VertexShaderID);
 OpenGL->glDeleteShader(FragmentShaderID);
 
 Result.ProgramHandle = ProgramID;
 Result.VertP_AttrLoc = OpenGL->glGetAttribLocation(ProgramID, "VertP");
 Result.VertUV_AttrLoc = OpenGL->glGetAttribLocation(ProgramID, "VertUV");
 Result.Transform_UniformLoc = OpenGL->glGetUniformLocation(ProgramID, "Transform");
 Result.Color_UniformLoc = OpenGL->glGetUniformLocation(ProgramID, "Color");
 
 return Result;
}

DLL_EXPORT
RENDERER_END_FRAME(Win32RendererEndFrame)
{
 opengl *OpenGL = Cast(opengl *)Renderer;
 texture_transfer_queue *Queue = &Memory->TextureQueue;
 GLuint *Textures = OpenGL->Textures;
 m4x4 Projection = M3x3ToM4x4OpenGL(Frame->Proj);
 
 v4 Clear = Frame->ClearColor;
 glClearColor(Clear.R, Clear.G, Clear.B, Clear.A);
 glClear(GL_COLOR_BUFFER_BIT);
 glViewport(0, 0, Frame->WindowDim.X, Frame->WindowDim.Y);
 
 //- uploads textures into GPU
 for (u32 OpIndex = 0;
      OpIndex < Queue->OpCount;
      ++OpIndex)
 {
  texture_transfer_op *Op = Queue->Ops + OpIndex;
  Assert(Op->TextureIndex < OpenGL->MaxTextureCount);
  
  glBindTexture(GL_TEXTURE_2D, Textures[Op->TextureIndex]);
  glTexImage2D(GL_TEXTURE_2D,
               0,
               GL_RGBA,
               Cast(u32)Op->Width,
               Cast(u32)Op->Height,
               0,
               GL_RGBA,
               GL_UNSIGNED_BYTE,
               Op->Pixels);
  OpenGL->glGenerateMipmap(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, 0);
 }
 Queue->OpCount = 0;
 Queue->TransferMemoryUsed = 0;
 
 //- draw
#if 1
 QuickSort(Frame->Commands, Frame->CommandCount, render_command, RenderCommandCmp);
 for (u32 CommandIndex = 0;
      CommandIndex < Frame->CommandCount;
      ++CommandIndex)
 {
  render_command *Command = Frame->Commands + CommandIndex;
  
  glMatrixMode(GL_PROJECTION);
  glLoadMatrixf(Cast(f32 *)Projection.M);
  
  m4x4 Model = M3x3ToM4x4OpenGL(Command->ModelXForm);
  glMatrixMode(GL_MODELVIEW);
  glLoadMatrixf(Cast(f32 *)Model.M);
  
  switch (Command->Type)
  {
   case RenderCommand_VertexArray: {
    render_command_vertex_array *Array = &Command->VertexArray;
    
    int glPrimitive = 0;
    switch (Array->Primitive)
    {
     case Primitive_TriangleStrip: {glPrimitive = GL_TRIANGLE_STRIP;}break;
     case Primitive_Triangles:     {glPrimitive = GL_TRIANGLES;}break;
    }
    
    glBegin(glPrimitive);
    glColor4fv(Array->Color.E);
    for (u32 VertexIndex = 0;
         VertexIndex < Array->VertexCount;
         ++VertexIndex)
    {
     glVertex2fv(Array->Vertices[VertexIndex].Pos.E);
    }
    glEnd();
   }break;
   
   case RenderCommand_Circle: {
    render_command_circle *Circle = &Command->Circle;
    
    glBegin(GL_TRIANGLE_FAN);
    glColor4fv(Circle->Color.E);
    glVertex2f(0.0f, 0.0f);
    u32 VertexCount = 30;
    for (u32 VertexIndex = 0;
         VertexIndex <= VertexCount;
         ++VertexIndex)
    {
     f32 Radians = 2*Pi32 / VertexCount * VertexIndex;
     v2 P = Rotation2DFromRadians(Radians);
     glVertex2fv(P.E);
    }
    glEnd();
    
    glBegin(GL_TRIANGLE_STRIP);
    glColor4fv(Circle->OutlineColor.E);
    for (u32 VertexIndex = 0;
         VertexIndex <= VertexCount;
         ++VertexIndex)
    {
     f32 Radians = 2*Pi32 / VertexCount * VertexIndex;
     v2 P = Rotation2DFromRadians(Radians);
     v2 OP = Circle->OutlineThickness * P;
     glVertex2fv(P.E);
     glVertex2fv(OP.E);
    }
    glEnd();
   }break;
   
   case RenderCommand_Rectangle: {
    render_command_rectangle *Rect = &Command->Rectangle;
    
    glBegin(GL_QUADS);
    glColor4fv(Rect->Color.E);
    glVertex2f(-1, -1);
    glVertex2f(1, -1);
    glVertex2f(1, 1);
    glVertex2f(-1, 1);
    glEnd();
   }break;
   
   case RenderCommand_Triangle: {
    render_command_triangle *Tri = &Command->Triangle;
    
    glBegin(GL_TRIANGLES);
    glColor4fv(Tri->Color.E);
    glVertex2fv(Tri->P0.E);
    glVertex2fv(Tri->P1.E);
    glVertex2fv(Tri->P2.E);
    glEnd();
   }break;
   
   case RenderCommand_Image: {
    render_command_image *Image = &Command->Image;
    
    glBindTexture(GL_TEXTURE_2D, Textures[Image->TextureIndex]);
    
    glBegin(GL_QUADS);
    glColor4f(1, 1, 1, 1);
    
    glTexCoord2f(0, 0);
    glVertex2f(-1, -1);
    
    glTexCoord2f(1, 0);
    glVertex2f(1, -1);
    
    glTexCoord2f(1, 1);
    glVertex2f(1, 1);
    
    glTexCoord2f(0, 1);
    glVertex2f(-1, 1);
    
    glEnd();
    
    glBindTexture(GL_TEXTURE_2D, 0);
   }break;
  }
 }
#endif
 
#if 0
 {
  v2 Vertices[] = {
   {-0.5f, -0.5f}, {0.0f, 0.0f},
   {+0.5f, -0.5f}, {1.0f, 0.0f},
   {-0.5f, +0.5f}, {0.0f, 1.0f},
   
   {+0.5f, -0.5f}, {1.0f, 0.0f},
   {+0.5f, +0.5f}, {1.0f, 1.0f},
   {-0.5f, +0.5f}, {0.0f, 1.0f},
  };
  
  v4 Color = V4(1, 0, 1, 1);
  m4x4 Transform = Projection;
  
  char const *VertexCode = R"FOO(
#version 330

in vec2 VertP;
in vec2 VertUV;

out vec2 FragUV;

uniform mat4 Transform;

void main(void) {
 gl_Position = Transform * vec4(VertP, 0, 1);
FragUV = VertUV;
}

)FOO";
  
  char const *FragmentCode = R"FOO(
#version 330

in vec2 FragUV;
out vec4 FragColor;

uniform vec4 Color;
uniform sampler2D Sampler;

void main(void) {
FragColor = texture(Sampler, FragUV);
//FragColor = Color;
}

)FOO";
  
  local b32 Init = false;
  local GLuint VAO = 0;
  local GLuint VBO = 0;
  local opengl_program Program = {};
  
  if (!Init)
  {
   Program = Win32OpenGLCreateProgram(OpenGL, Cast(char *)VertexCode, Cast(char *)FragmentCode);
   OpenGL->glGenVertexArrays(1, &VAO);
   OpenGL->glGenBuffers(1, &VBO);
   Init = true;
  }
  
#if 1
  OpenGL->glBindVertexArray(VAO);
  OpenGL->glBindBuffer(GL_ARRAY_BUFFER, VBO);
  OpenGL->glBufferData(GL_ARRAY_BUFFER, SizeOf(Vertices), Vertices, GL_DYNAMIC_DRAW);
  
  OpenGL->glEnableVertexAttribArray(Program.VertP_AttrLoc);
  OpenGL->glVertexAttribPointer(Program.VertP_AttrLoc, 2, GL_FLOAT, GL_FALSE, 2 * SizeOf(v2), 0);
  OpenGL->glEnableVertexAttribArray(Program.VertUV_AttrLoc);
  OpenGL->glVertexAttribPointer(Program.VertUV_AttrLoc, 2, GL_FLOAT, GL_FALSE, 2 * SizeOf(v2), Cast(void *)SizeOf(v2));
  
  OpenGL->glUseProgram(Program.ProgramHandle);
  OpenGL->glUniformMatrix4fv(Program.Transform_UniformLoc, 1, GL_FALSE, Cast(f32 *)Transform.M);
  OpenGL->glUniform4fv(Program.Color_UniformLoc, 1, Cast(f32 *)Color.E);
  
  OpenGL->glDrawArrays(GL_TRIANGLES, 0, ArrayCount(Vertices));
  
  OpenGL->glUseProgram(0);
  OpenGL->glBindBuffer(GL_ARRAY_BUFFER, 0);
  OpenGL->glBindVertexArray(0);
#endif
 }
#endif
 
 Memory->ImGuiBindings.Render();
 SwapBuffers(OpenGL->WindowDC);
}