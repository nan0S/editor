internal sfml_renderer *
SFMLInit(arena *Arena, sf::RenderWindow *Window)
{
   sfml_renderer *SFML = PushStruct(Arena, sfml_renderer);
   SFML->Window = Window;
   
   return SFML;
}

internal render_commands *
SFMLBeginFrame(sfml_renderer *SFML)
{
   render_commands *Commands = &SFML->Commands;
   Commands->CommandCount = 0;
   Commands->Commands = SFML->CommandBuffer;
   Commands->MaxCommandCount = ArrayCount(SFML->CommandBuffer);
   
   return Commands;
}

internal void
SFMLEndFrame(sfml_renderer *SFML, render_commands *Commands)
{
   sf::RenderWindow *Window = SFML->Window;
   for (u64 CommandIndex = 0;
        CommandIndex < Commands->CommandCount;
        ++CommandIndex)
   {
      render_command *Command = Commands->Commands + CommandIndex;
      
      sf::Transform Transform = {};
      {
         render_transform *XForm = &Command->XForm;
         Transform = Transform
            .rotate(Rotation2DToDegrees(XForm->Rotation))
            .scale(XForm->Scale.X, XForm->Scale.Y)
            .translate(-V2ToVector2f(XForm->Offset));
      }
      
      switch (Command->Type)
      {
         case RenderCommand_Clear: {
            v4 Color = Command->Clear.Color;
            Window->clear(ColorToSFMLColor(Color));
         }break;
         
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
            
            Window->draw(SFMLVertices, Array->VertexCount, Primitive, Transform);
            
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
   
   {
      TimeBlock("ImGui::SFML::Render");
      ImGui::SFML::Render(*Window);
   }
   
   {
      TimeBlock("Window->display");
      Window->display();
   }
}

internal render_command *
PushRenderCommand(render_commands *Commands, render_transform XForm)
{
   Assert(Commands->CommandCount < Commands->MaxCommandCount);
   render_command *Result = Commands->Commands + Commands->CommandCount;
   ++Commands->CommandCount;
   Result->XForm = XForm;
   
   return Result;
}

internal void
PushVertexArray(render_commands *Commands,
                vertex *Vertices,
                u64 VertexCount,
                render_primitive_type Primitive,
                v4 Color,
                render_transform XForm)
{
   render_command *Command = PushRenderCommand(Commands, XForm);
   Command->Type = RenderCommand_VertexArray;
   render_command_vertex_array *Array = &Command->VertexArray;
   Array->Vertices = Vertices;
   Array->Vertices = Vertices;
   Array->VertexCount = VertexCount;
   Array->Primitive = Primitive;
   Array->Color = Color;
}

internal void
PushClearColor(render_commands *Commands, v4 Color)
{
   render_command *Command = PushRenderCommand(Commands, Identity());
   Command->Type = RenderCommand_Clear;
   Command->Clear.Color = Color;
}

internal void
PushCircle(render_commands *Commands,
           v2 Position, f32 Radius,
           v4 Color, render_transform XForm,
           f32 OutlineThickness = 0.0f,
           v4 OutlineColor = V4(0.0f, 0.0f, 0.0f, 0.0f))
{
   render_command *Command = PushRenderCommand(Commands, XForm);
   Command->Type = RenderCommand_Circle;
   render_command_circle *Circle = &Command->Circle;
   Circle->Pos = Position;
   Circle->Radius = Radius;
   Circle->Color = Color;
   Circle->OutlineThickness = OutlineThickness;
   Circle->OutlineColor = OutlineColor;
}

internal void
PushRectangle(render_commands *Commands,
              v2 Position, v2 Size, rotation_2d Rotation,
              v4 Color, render_transform XForm)
{
   render_command *Command = PushRenderCommand(Commands, XForm);
   Command->Type = RenderCommand_Rectangle;
   render_command_rectangle *Rectangle = &Command->Rectangle;
   Rectangle->Pos = Position;
   Rectangle->Size = Size;
   Rectangle->Rotation = Rotation;
   Rectangle->Color = Color;
}

internal void
PushSquare(render_commands *Commands, v2 Position, f32 Side, v4 Color, render_transform XForm)
{
   PushRectangle(Commands, Position, V2(Side, Side), Rotation2DZero(), Color, XForm);
}

internal void
PushLine(render_commands *Commands,
         v2 BeginPoint, v2 EndPoint,
         f32 LineWidth, v4 Color,
         render_transform XForm)
{
   v2 Position = 0.5f * (BeginPoint + EndPoint);
   v2 Line = EndPoint - BeginPoint;
   f32 Length = Norm(Line);
   v2 Size = V2(LineWidth, Length);
   // NOTE(hbr): Rotate 90 degrees clockwise, because our 0 degree rotation
   // corresponds to -90 degrees rotation in the real world
   rotation_2d Rotation = Rotate90DegreesClockwise(Rotation2DFromVector(Line));
   PushRectangle(Commands, Position, Size, Rotation, Color, XForm);
}

internal void
PushTriangle(render_commands *Commands,
             v2 P0, v2 P1, v2 P2,
             v4 Color, render_transform XForm)
{
   render_command *Command = PushRenderCommand(Commands, XForm);
   Command->Type = RenderCommand_Triangle;
   render_command_triangle *Triangle = &Command->Triangle;
   Triangle->P0 = P0;
   Triangle->P1 = P1;
   Triangle->P2 = P2;
   Triangle->Color = Color;
}
