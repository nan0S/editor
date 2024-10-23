internal sfml_renderer *
InitSFMLRenderer(arena *Arena, sf::RenderWindow *Window)
{
   sfml_renderer *SFML = PushStruct(Arena, sfml_renderer);
   SFML->Window = Window;
   
   return SFML;
}

internal render_frame *
SFMLBeginFrame(sfml_renderer *Renderer)
{
   render_frame *Frame = &Renderer->Frame;
   Frame->CommandCount = 0;
   Frame->Commands = Renderer->CommandBuffer;
   Frame->MaxCommandCount = ArrayCount(Renderer->CommandBuffer);
   
   sf::Vector2u WindowSize = Renderer->Window->getSize();
   Frame->WindowDim = V2S32(WindowSize.x, WindowSize.y);
   
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

internal void
SFMLEndFrame(sfml_renderer *Renderer, render_frame *Frame)
{
   TimeFunction;
   
   sf::Transform Transform = RenderTransformToSFMLTransform(Frame->Proj);
   
   sf::RenderWindow *Window = Renderer->Window;
   
   // NOTE(hbr): Set Normalized Device Coordinates View
   sf::Vector2f Size = sf::Vector2f(2.0f, -2.0f);
   sf::Vector2f Center = sf::Vector2f(0.0f, 0.0f);
   sf::View View = sf::View(Center, Size);
   Window->setView(View);
   
   Window->clear(ColorToSFMLColor(Frame->ClearColor));
   
   QuickSort(Frame->Commands, Frame->CommandCount, render_command, RenderCommandCmp);
   
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
            
            temp_arena Temp = TempArena(0);
            sf::Vertex *SFMLVertices = PushArrayNonZero(Temp.Arena, Array->VertexCount, sf::Vertex);
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
            
            EndTemp(Temp);
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
      }
   }
   
   ImGui::SFML::Render(*Window);
   Window->display();
}
