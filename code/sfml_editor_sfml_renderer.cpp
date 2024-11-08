#include "imgui_inc.h"

#include "editor_base.h"
#include "editor_memory.h"
#include "editor_string.h"
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

internal sf::Transform
RenderTransformToSFMLTransform(transform A)
{
 sf::Transform Result = sf::Transform()
  .translate(A.Offset.X, A.Offset.Y)
  .scale(A.Scale.X, A.Scale.Y)
  .rotate(Rotation2DToDegrees(A.Rotation));
 
 return Result;
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
 sf::Transform Transform = RenderTransformToSFMLTransform(Frame->Proj);
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
  glLoadMatrixf(Transform.getMatrix());
  
  switch (Command->Type)
  {
   case RenderCommand_VertexArray: {
    render_command_vertex_array *Array = &Command->VertexArray;
    
    sf::Transform Model = RenderTransformToSFMLTransform(Array->ModelXForm);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(Model.getMatrix());
    
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
     sf::Transform Model = RenderTransformToSFMLTransform(Tmp);
     glMatrixMode(GL_MODELVIEW);
     glLoadMatrixf(Model.getMatrix());
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
    sf::Transform Model = RenderTransformToSFMLTransform(Tmp);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(Model.getMatrix());
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
    
    sf::Transform Model;
    {
     transform Tmp = {};
     Tmp.Offset = Image->P;
     Tmp.Rotation = Image->Rotation;
     Tmp.Scale = Image->Scale;
     Model = RenderTransformToSFMLTransform(Tmp);
    }
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(Model.getMatrix());
    
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