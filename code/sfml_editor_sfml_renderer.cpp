#include "third_party/sfml/include/SFML/Graphics.hpp"
#include "third_party/sfml/include/SFML/OpenGL.hpp"

#include "imgui_inc.h"

#include "editor_base.h"
#include "editor_memory.h"
#include "editor_string.h"
#include "editor_math.h"
#include "editor_platform.h"
#include "editor_renderer.h"
#include "editor_renderer_sfml.h"
#include "sfml_editor_renderer.h"

#include "editor_math.cpp"
#include "editor_memory.cpp"

#include "third_party/imgui_sfml/imgui-SFML.h"

/* TODO(hbr):
I don't want:
- editor_memory
- most of editor_math
- editor_string
- editor_os
*/

internal sfml_renderer *
SFMLRendererInitImpl(arena *Arena, platform_renderer_limits *Limits, sf::RenderWindow *Window)
{
 sfml_renderer *Renderer = PushStruct(Arena, sfml_renderer);
 
 texture_transfer_queue *Queue = &Renderer->PlatformHeader.TextureQueue;
 Queue->TransferMemorySize = Limits->MaxTextureQueueMemorySize;
 Queue->TransferMemory = PushArrayNonZero(Arena, Queue->TransferMemorySize, char);
 
 Renderer->Window = Window;
 
 u64 TextureCount = Limits->MaxTextureCount;
 GLuint *Textures = PushArray(Arena, TextureCount, GLuint);
 Renderer->MaxTextureCount = TextureCount;
 Renderer->Textures = Textures;
 
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
 
 return Renderer;
}

SFML_RENDERER_INIT(SFMLRendererInit)
{
 platform_renderer *Renderer = Cast(platform_renderer *)SFMLRendererInitImpl(Arena, Limits, Window);
 return Renderer;
}

internal render_frame *
SFMLBeginFrameImpl(sfml_renderer *Renderer, v2u WindowDim)
{
 render_frame *Frame = &Renderer->Frame;
 Frame->CommandCount = 0;
 Frame->Commands = Renderer->CommandBuffer;
 Frame->MaxCommandCount = ArrayCount(Renderer->CommandBuffer);
 Frame->WindowDim = WindowDim;
 
#if 0
 ImGui_ImplOpenGL3_NewFrame();
 ImGui_ImplWin32_NewFrame();
 ImGui::NewFrame();
#endif
 
 return Frame;
}

RENDERER_BEGIN_FRAME(SFMLBeginFrame)
{
 render_frame *Frame = SFMLBeginFrameImpl(Cast(sfml_renderer *)Renderer, WindowDim);
 return Frame;
}

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

internal int
SFMLRenderCommandCmp(render_command *A, render_command *B)
{
 return Cmp(A->ZOffset, B->ZOffset);
}

internal void
SFMLEndFrameImpl(sfml_renderer *Renderer, render_frame *Frame)
{
 GLuint *Textures = Renderer->Textures;
 m4x4 M = M3x3ToM4x4OpenGL(Frame->Proj);
 sf::RenderWindow *Window = Renderer->Window;
 
 {
  v4 Clear = Frame->ClearColor;
  glClearColor(Clear.R, Clear.G, Clear.B, Clear.A);
  glClear(GL_COLOR_BUFFER_BIT);
 }
 
 //- uploads textures into GPU
 {
  texture_transfer_queue *Queue = &Renderer->PlatformHeader.TextureQueue;
  for (u64 OpIndex = 0;
       OpIndex < Queue->OpCount;
       ++OpIndex)
  {
   texture_transfer_op *Op = Queue->Ops + OpIndex;
   Assert(Op->TextureIndex < Renderer->MaxTextureCount);
   
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
 }
 
 QuickSort(Frame->Commands, Frame->CommandCount, render_command, SFMLRenderCommandCmp);
 
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
 
#if 0
 ImGui::Render();
 ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
 
 // TODO(hbr): Of course remove this from here and instead write this as a platform agnostic layer
 HWND WindowHandle = GetActiveWindow();
 HDC DeviceContext = GetDC(WindowHandle);
 SwapBuffers(DeviceContext);
#endif
 
 ImGui::SFML::Render(*Window);
 Window->display();
}

RENDERER_END_FRAME(SFMLEndFrame)
{
 SFMLEndFrameImpl(Cast(sfml_renderer *)Renderer, Frame);
}