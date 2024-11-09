#include "imgui_inc.h"

#include "editor_base.h"
#include "editor_string.h"
#include "editor_platform.h"
#include "editor_memory.h"
#include "editor_os.h"
#include "editor_renderer.h"
#include "editor_math.h"
#include "editor_sort.h"

#include "editor_base.cpp"
#include "editor_memory.cpp"
#include "editor_string.cpp"
#include "editor_os.cpp"
#include "editor_math.cpp"

#include "third_party/sfml/include/SFML/Graphics.hpp"
#include "third_party/sfml/include/SFML/OpenGL.hpp"

#include "sfml_editor_renderer.h"
#include "editor_renderer_sfml.h"

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
 
 return Frame;
}

RENDERER_BEGIN_FRAME(SFMLBeginFrame)
{
 render_frame *Frame = SFMLBeginFrameImpl(Cast(sfml_renderer *)Renderer, WindowDim);
 return Frame;
}

internal m3x3
M3x3Identity(void)
{
 m3x3 Result = {};
 Result.M[0][0] = 1.0f;
 Result.M[1][1] = 1.0f;
 Result.M[2][2] = 1.0f;
 
 return Result;
}

internal m3x3
M3x3Mult(m3x3 A, m3x3 B)
{
 m3x3 Result = {};
 for (u64 i = 0; i < 3; ++i)
 {
  for (u64 j = 0; j < 3; ++j)
  {
   for (u64 k = 0; k < 3; ++k)
   {
    Result.M[i][j] += A.M[i][k] * B.M[k][j];
   }
  }
 }
 
 return Result;
}

internal m3x3
M3x3Transpose(m3x3 M)
{
 m3x3 Result = M;
 for (u64 i = 0; i < 3; ++i)
 {
  for (u64 j = 0; j < i; ++j)
  {
   Swap(Result.M[i][j], Result.M[j][i], f32);
  }
 }
 
 return Result;
}


struct m4x4
{
 f32 M[4][4];
};

internal m4x4
M3x3ToM4x4OpenGL(m3x3 M)
{
 M = M3x3Transpose(M);
 
#define X(i,j) M.M[i][j]
 m4x4 R = {
  { {X(0,0), X(0,1), 0, X(0,2)},
   {X(1,0), X(1,1), 0, X(1,2)},
   {     0,      0, 1,      0},
   {X(2,0), X(2,1), 0, X(2,2)}}
 };
#undef X
 
 return R;
}

internal m4x4
TransformToM3x3(transform T)
{
 m3x3 A = M3x3Identity();
 A.M[0][2] = T.Offset.X;
 A.M[1][2] = T.Offset.Y;
 A.M[2][2] = 1.0f;
 
 m3x3 B = M3x3Identity();
 B.M[0][0] = T.Scale.X;
 B.M[1][1] = T.Scale.Y;
 B.M[2][2] = 1.0f;
 
 m3x3 C = M3x3Identity();
 C.M[0][0] = T.Rotation.X;
 C.M[1][0] = T.Rotation.Y;
 C.M[0][1] = -T.Rotation.Y;
 C.M[1][1] = T.Rotation.X;
 C.M[2][2] = 1.0f;
 
 m3x3 Result = M3x3Identity();
 Result = M3x3Mult(Result, A);
 Result = M3x3Mult(Result, B);
 Result = M3x3Mult(Result, C);
 
 m4x4 R = M3x3ToM4x4OpenGL(Result);
 
 return R;
}

internal int
SFMLRenderCommandCmp(render_command *A, render_command *B)
{
 return Cmp(A->ZOffset, B->ZOffset);
}

internal inline sf::Color
ColorToSFMLColor(v4 Color)
{
 sf::Color Result = {};
 Result.r = Cast(u8)(255 * ClampTop(Color.R, 1.0f));
 Result.g = Cast(u8)(255 * ClampTop(Color.G, 1.0f));
 Result.b = Cast(u8)(255 * ClampTop(Color.B, 1.0f));
 Result.a = Cast(u8)(255 * ClampTop(Color.A, 1.0f));
 
 return Result;
}

internal void
SFMLEndFrameImpl(sfml_renderer *Renderer, render_frame *Frame)
{
 GLuint *Textures = Renderer->Textures;
 m4x4 M = TransformToM3x3(Frame->Proj);
 sf::RenderWindow *Window = Renderer->Window;
 Window->clear(ColorToSFMLColor(Frame->ClearColor));
 
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
  
  switch (Command->Type)
  {
   case RenderCommand_VertexArray: {
    render_command_vertex_array *Array = &Command->VertexArray;
    
    m4x4 Model = TransformToM3x3(Array->ModelXForm);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(Cast(f32 *)Model.M);
    
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
    
    {
     transform Tmp = {};
     Tmp.Offset = Circle->Pos;
     Tmp.Scale = V2(1.0f, 1.0f);
     Tmp.Rotation = Rotation2DZero();
     m4x4 Model = TransformToM3x3(Tmp);
     glMatrixMode(GL_MODELVIEW);
     glLoadMatrixf(Cast(f32 *)Model.M);
    }
    
    glBegin(GL_TRIANGLE_FAN);
    glColor4fv(Circle->Color.E);
    glVertex2f(0.0f, 0.0f);
    u64 VertexCount = 30;
    for (u64 VertexIndex = 0;
         VertexIndex <= VertexCount;
         ++VertexIndex)
    {
     f32 Radians = 2*Pi32 / VertexCount * VertexIndex;
     v2 P = Circle->Radius * Rotation2DFromRadians(Radians);
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
     v2 P1 = Circle->Radius * P;
     v2 P2 = (Circle->Radius + Circle->OutlineThickness) * P;
     glVertex2fv(P1.E);
     glVertex2fv(P2.E);
    }
    glEnd();
   }break;
   
   case RenderCommand_Rectangle: {
    render_command_rectangle *Rect = &Command->Rectangle;
    
    transform Tmp = {};
    Tmp.Offset = Rect->Pos;
    Tmp.Rotation = Rect->Rotation;
    Tmp.Scale = 0.5f * Rect->Size;
    m4x4 Model = TransformToM3x3(Tmp);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(Cast(f32 *)Model.M);
    
    glBegin(GL_QUADS);
    glColor4fv(Rect->Color.E);
    glVertex2f(-1.0f, -1.0f);
    glVertex2f(+1.0f, -1.0f);
    glVertex2f(+1.0f, +1.0f);
    glVertex2f(-1.0f, +1.0f);
    glEnd();
   }break;
   
   case RenderCommand_Triangle: {
    render_command_triangle *Tri = &Command->Triangle;
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
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
    
    m4x4 Model;
    {
     transform Tmp = {};
     Tmp.Offset = Image->P;
     Tmp.Rotation = Image->Rotation;
     Tmp.Scale = Image->Scale;
     Model = TransformToM3x3(Tmp);
    }
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(Cast(f32 *)Model.M);
    
    glBegin(GL_QUADS);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(-0.5f * Image->Dim.X, -0.5f * Image->Dim.Y);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(+0.5f * Image->Dim.X, -0.5f * Image->Dim.Y);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(+0.5f * Image->Dim.X, +0.5f * Image->Dim.Y);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(-0.5f * Image->Dim.X, +0.5f * Image->Dim.Y);
    glEnd();
    
    glBindTexture(GL_TEXTURE_2D, 0);
   }break;
  }
 }
 
 ImGui::SFML::Render(*Window);
 Window->display();
}

RENDERER_END_FRAME(SFMLEndFrame)
{
 SFMLEndFrameImpl(Cast(sfml_renderer *)Renderer, Frame);
}