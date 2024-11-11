#include <windows.h>
#include <gl/gl.h>

#include "imgui_inc.h"

#include "editor_base.h"
#include "editor_memory.h"
#include "editor_string.h"
#include "editor_math.h"
#include "editor_platform.h"
#include "editor_renderer.h"
#include "win32_editor_renderer.h"

#include "editor_memory.cpp"
#include "editor_math.cpp"

platform_api Platform;

struct opengl
{
 platform_renderer PlatformHeader;
 
 render_frame RenderFrame;
 
 render_command *CommandBuffer;
 u64 MaxCommandCount;
 
 u64 MaxTextureCount;
 GLuint *Textures;
};

WIN32_RENDERER_INIT()
{
 opengl *OpenGL = PushStruct(Arena, opengl);
 HDC WindowDC = GetDC(Window);
 
 OpenGL->CommandBuffer = PushArrayNonZero(Arena, Limits->MaxCommandCount, render_command);
 OpenGL->MaxCommandCount = Limits->MaxCommandCount;
 
 //- set pixel format
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
  
  int SuggestedPixelFormatIndex = ChoosePixelFormat(WindowDC, &DesiredPixelFormat);
  
  PIXELFORMATDESCRIPTOR SuggestedPixelFormat;
  DescribePixelFormat(WindowDC, SuggestedPixelFormatIndex, SizeOf(SuggestedPixelFormat), &SuggestedPixelFormat);
  SetPixelFormat(WindowDC, SuggestedPixelFormatIndex, &SuggestedPixelFormat);
 }
 
 //- create opengl context
 HGLRC OpenGLRC = wglCreateContext(WindowDC);
 if (wglMakeCurrent(WindowDC, OpenGLRC))
 {
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
 }
 
 //- allocate texture transfer queue
 {
  texture_transfer_queue *Queue = &OpenGL->PlatformHeader.TextureQueue;
  Queue->TransferMemorySize = Limits->MaxTextureQueueMemorySize;
  Queue->TransferMemory = PushArrayNonZero(Arena, Queue->TransferMemorySize, char);
 }
 
 //- allocate texutre indices
 u64 TextureCount = Limits->MaxTextureCount;
 GLuint *Textures = PushArray(Arena, TextureCount, GLuint);
 OpenGL->MaxTextureCount = TextureCount;
 OpenGL->Textures = Textures;
 
 glGenTextures(TextureCount, Textures);
 for (u64 TextureIndex = 0;
      TextureIndex < TextureCount;
      ++TextureIndex)
 {
  glBindTexture(GL_TEXTURE_2D, Textures[TextureIndex]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
 }
 
 //- initialize ImGui
 ImGui_ImplWin32_Init(Window);
 ImGui_ImplOpenGL3_Init();
 
 return Cast(platform_renderer *)OpenGL;
}

WIN32_RENDERER_BEGIN_FRAME()
{
 opengl *OpenGL = Cast(opengl *)Renderer;
 
 render_frame *RenderFrame = &OpenGL->RenderFrame;
 RenderFrame->CommandCount = 0;
 RenderFrame->Commands = OpenGL->CommandBuffer;
 RenderFrame->MaxCommandCount = OpenGL->MaxCommandCount;
 RenderFrame->WindowDim = WindowDim;
 
 ImGui_ImplOpenGL3_NewFrame();
 ImGui_ImplWin32_NewFrame();
 ImGui::NewFrame();
 
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

WIN32_RENDERER_END_FRAME()
{
 opengl *OpenGL = Cast(opengl *)Renderer;
 texture_transfer_queue *Queue = &OpenGL->PlatformHeader.TextureQueue;
 GLuint *Textures = OpenGL->Textures;
 m4x4 M = M3x3ToM4x4OpenGL(Frame->Proj);
 
 v4 Clear = Frame->ClearColor;
 glClearColor(Clear.R, Clear.G, Clear.B, Clear.A);
 glClear(GL_COLOR_BUFFER_BIT);
 glViewport(0, 0, Frame->WindowDim.X, Frame->WindowDim.Y);
 
 //- uploads textures into GPU
 for (u64 OpIndex = 0;
      OpIndex < Queue->OpCount;
      ++OpIndex)
 {
  texture_transfer_op *Op = Queue->Ops + OpIndex;
  Assert(Op->TextureIndex < OpenGL->MaxTextureCount);
  
  glBindTexture(GL_TEXTURE_2D, Textures[Op->TextureIndex]);
  glTexImage2D(GL_TEXTURE_2D,
               0,
               GL_RGBA,
               Op->Width, Op->Height,
               0,
               GL_RGBA,
               GL_UNSIGNED_BYTE,
               Op->Pixels);
 }
 Queue->OpCount = 0;
 Queue->TransferMemoryUsed = 0;
 
 //- draw
 QuickSort(Frame->Commands, Frame->CommandCount, render_command, RenderCommandCmp);
 for (u64 CommandIndex = 0;
      CommandIndex < Frame->CommandCount;
      ++CommandIndex)
 {
  render_command *Command = Frame->Commands + CommandIndex;
  
  glMatrixMode(GL_PROJECTION);
  glLoadMatrixf(Cast(f32 *)M.M);
  
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
    for (u64 VertexIndex = 0;
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
    u64 VertexCount = 30;
    for (u64 VertexIndex = 0;
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
    for (u64 VertexIndex = 0;
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
 
 ImGui::Render();
 ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
 SwapBuffers(wglGetCurrentDC());
}