#include "imgui_inc.h"

#include "editor_base.h"
#include "editor_memory.h"
#include "editor_string.h"
#include "editor_os.h"
#include "editor_renderer.h"
#include "editor_math.h"

#include "editor_base.cpp"
#include "editor_memory.cpp"
#include "editor_string.cpp"
#include "editor_os.cpp"
#include "editor_math.cpp"

#include "third_party/sfml/include/SFML/Graphics.hpp"
#include "sfml_editor_renderer.h"

#include <malloc.h>

/* TODO(hbr):
I don't want:
- editor_memory
- most of editor_math
- editor_string
- editor_os
- malloc
*/

internal sfml_renderer *
SFMLRendererInitImpl(sf::RenderWindow *Window)
{
 sfml_renderer *SFML = Cast(sfml_renderer *)malloc(SizeOf(sfml_renderer));
 SFML->Window = Window;
 
 return SFML;
}

SFML_RENDERER_INIT(SFMLRendererInit)
{
 platform_renderer *Renderer = Cast(platform_renderer *)SFMLRendererInitImpl(Window);
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

internal inline sf::Vector2f
V2ToVector2f(v2 V)
{
 sf::Vector2f Result = {};
 Result.x = V.X;
 Result.y = V.Y;
 
 return Result;
}

internal inline sf::Vector2i
V2S32ToVector2i(v2s V)
{
 sf::Vector2i Result = {};
 Result.x = V.X;
 Result.y = V.Y;
 
 return Result;
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
 sf::Transform Transform = RenderTransformToSFMLTransform(Frame->Proj);
 
 sf::RenderWindow *Window = Renderer->Window;
 
 // NOTE(hbr): Set Normalized Device Coordinates View
 sf::Vector2f Size = sf::Vector2f(2.0f, -2.0f);
 sf::Vector2f Center = sf::Vector2f(0.0f, 0.0f);
 sf::View View = sf::View(Center, Size);
 Window->setView(View);
 
 Window->clear(ColorToSFMLColor(Frame->ClearColor));
 
 QuickSort(Frame->Commands, Frame->CommandCount, render_command, SFMLRenderCommandCmp);
 
 for (u64 CommandIndex = 0;
      CommandIndex < Frame->CommandCount;
      ++CommandIndex)
 {
  render_command *Command = Frame->Commands + CommandIndex;
  switch (Command->Type)
  {
   case RenderCommand_VertexArray: {
    render_command_vertex_array *Array = &Command->VertexArray;
    
    sf::PrimitiveType Primitive = {};
    switch (Array->Primitive)
    {
     case Primitive_Triangles: { Primitive = sf::Triangles; }break;
     case Primitive_TriangleStrip: { Primitive = sf::TriangleStrip; }break;
    }
    
    sf::Vertex *SFMLVertices = Cast(sf::Vertex *)malloc(Array->VertexCount * SizeOf(sf::Vertex));
    {
     vertex *Vertices = Array->Vertices;
     v4 Color = Array->Color;
     for (u64 VertexIndex = 0;
          VertexIndex < Array->VertexCount;
          ++VertexIndex)
     {
      sf::Vertex *V = SFMLVertices + VertexIndex;
      V->position = V2ToVector2f(Vertices[VertexIndex].Pos);
      V->color = ColorToSFMLColor(Color);
     }
    }
    
    sf::Transform Model = RenderTransformToSFMLTransform(Array->ModelXForm);
    sf::Transform MVP = Transform * Model;
    
    Window->draw(SFMLVertices, Array->VertexCount, Primitive, MVP);
    
    free(SFMLVertices);
   }break;
   
   case RenderCommand_Circle: {
    render_command_circle *Circle = &Command->Circle;
    sf::CircleShape Shape = sf::CircleShape();
    Shape.setRadius(Circle->Radius);
    Shape.setFillColor(ColorToSFMLColor(Circle->Color));
    Shape.setOrigin(Circle->Radius, Circle->Radius);
    Shape.setPosition(Circle->Pos.X, Circle->Pos.Y);
    Shape.setOutlineThickness(Circle->OutlineThickness);
    Shape.setOutlineColor(ColorToSFMLColor(Circle->OutlineColor));
    Window->draw(Shape, Transform);
   }break;
   
   case RenderCommand_Rectangle: {
    render_command_rectangle *Rect = &Command->Rectangle;
    sf::RectangleShape Shape = sf::RectangleShape();
    Shape.setSize(V2ToVector2f(Rect->Size));
    Shape.setFillColor(ColorToSFMLColor(Rect->Color));
    Shape.setOrigin(0.5f * Rect->Size.X, 0.5f * Rect->Size.Y);
    Shape.setPosition(Rect->Pos.X, Rect->Pos.Y);
    Shape.setRotation(Rotation2DToDegrees(Rect->Rotation));
    Window->draw(Shape, Transform);
   }break;
   
   case RenderCommand_Triangle: {
    render_command_triangle *Tri = &Command->Triangle;
    sf::Vertex Verts[3] = {};
    Verts[0].position = V2ToVector2f(Tri->P0);
    Verts[1].position = V2ToVector2f(Tri->P1);
    Verts[2].position = V2ToVector2f(Tri->P2);
    Verts[0].color = ColorToSFMLColor(Tri->Color);
    Verts[1].color = ColorToSFMLColor(Tri->Color);
    Verts[2].color = ColorToSFMLColor(Tri->Color);
    Window->draw(Verts, 3, sf::Triangles, Transform);
   }break;
   
   case RenderCommand_Image: {
    render_command_image *Image = &Command->Image;
    
    sf::Image SFMLImage;
    sf::Texture SFMLTexture;
    sf::Sprite Sprite;
    SFMLImage.create(Image->Width, Image->Height, Cast(sf::Uint8 *)Image->Pixels);
    SFMLTexture.loadFromImage(SFMLImage);
    
    Sprite.setTexture(SFMLTexture);
    Sprite.setOrigin(0.5f * Image->Width, 0.5f * Image->Height);
    Sprite.setScale(1.0f / Image->Width * Image->Dim.X * Image->Scale.X, 1.0f / Image->Height * Image->Dim.Y * Image->Scale.Y);
    Sprite.setRotation(Rotation2DToDegrees(Image->Rotation));
    Sprite.setPosition(Image->P.X, Image->P.Y);
    
    Window->draw(Sprite, Transform);
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