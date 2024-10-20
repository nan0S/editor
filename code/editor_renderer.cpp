internal sfml_renderer *
SFMLInit(arena *Arena, sf::RenderWindow *Window)
{
   sfml_renderer *SFML = PushStruct(Arena, sfml_renderer);
   SFML->Window = Window;
   
   return SFML;
}

internal render_frame *
SFMLBeginFrame(sfml_renderer *SFML)
{
   render_frame *Frame = &SFML->Frame;
   Frame->CommandCount = 0;
   Frame->Commands = SFML->CommandBuffer;
   Frame->MaxCommandCount = ArrayCount(SFML->CommandBuffer);
   
   return Frame;
}

internal sf::Transform
RenderTransformToSFMLTransform(render_transform *XForm)
{
   f32 Rotation = Rotation2DToDegrees(Rotation2DInverse(XForm->Rotation));
   sf::Transform Transform =
      sf::Transform()
      .rotate(Rotation)
      .scale(XForm->Scale.X, XForm->Scale.Y)
      .translate(-XForm->Offset.X, -XForm->Offset.Y);
   
   return Transform;
}

internal void
SFMLEndFrame(sfml_renderer *SFML, render_frame *Frame)
{
   TimeFunction;
   
   sf::Transform Transform = RenderTransformToSFMLTransform(&Frame->Proj);
   
   sf::RenderWindow *Window = SFML->Window;
   
   // NOTE(hbr): Set Normalized Device Coordinates View
   sf::Vector2f Size = sf::Vector2f(2.0f, -2.0f);
   sf::Vector2f Center = sf::Vector2f(0.0f, 0.0f);
   sf::View View = sf::View(Center, Size);
   Window->setView(View);
   
   Window->clear(ColorToSFMLColor(Frame->ClearColor));
   
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
            
            sf::Transform Model = RenderTransformToSFMLTransform(&Array->ModelXForm);
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

internal render_command *
PushRenderCommand(render_group *Group, render_command_type Type)
{
   render_frame *Frame = Group->Frame;
   Assert(Frame->CommandCount < Frame->MaxCommandCount);
   render_command *Result = Frame->Commands + Frame->CommandCount;
   ++Frame->CommandCount;
   Result->Type = Type;
   
   return Result;
}

internal void
PushVertexArray(render_group *Group,
                vertex *Vertices,
                u64 VertexCount,
                render_primitive_type Primitive,
                v4 Color,
                render_transform ModelXForm)
{
   render_command *Command = PushRenderCommand(Group, RenderCommand_VertexArray);
   
   render_command_vertex_array *Array = &Command->VertexArray;
   Array->Vertices = Vertices;
   Array->VertexCount = VertexCount;
   Array->Primitive = Primitive;
   Array->Color = Color;
   Array->ModelXForm = ModelXForm;
}

internal void
PushCircle(render_group *Group,
           v2 Position, f32 Radius, v4 Color,
           render_transform XForm,
           f32 OutlineThickness = 0.0f,
           v4 OutlineColor = V4(0.0f, 0.0f, 0.0f, 0.0f))
{
   render_command *Command = PushRenderCommand(Group, RenderCommand_Circle);
   
   render_command_circle *Circle = &Command->Circle;
   Circle->Pos = Position;
   Circle->Radius = Radius;
   Circle->Color = Color;
   Circle->OutlineThickness = OutlineThickness;
   Circle->OutlineColor = OutlineColor;
}

internal void
PushRectangle(render_group *Group,
              v2 Position, v2 Size, rotation_2d Rotation,
              v4 Color, render_transform XForm)
{
   render_command *Command = PushRenderCommand(Group, RenderCommand_Rectangle);
   
   render_command_rectangle *Rectangle = &Command->Rectangle;
   Rectangle->Pos = Position;
   Rectangle->Size = Size;
   Rectangle->Rotation = Rotation;
   Rectangle->Color = Color;
}

internal void
PushSquare(render_group *Group, v2 Position, f32 Side, v4 Color, render_transform XForm)
{
   PushRectangle(Group, Position, V2(Side, Side), Rotation2DZero(), Color, XForm);
}

internal void
PushLine(render_group *Group,
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
   PushRectangle(Group, Position, Size, Rotation, Color, XForm);
}

internal void
PushTriangle(render_group *Group,
             v2 P0, v2 P1, v2 P2,
             v4 Color,
             render_transform XForm)
{
   render_command *Command = PushRenderCommand(Group, RenderCommand_Triangle);
   
   render_command_triangle *Triangle = &Command->Triangle;
   Triangle->P0 = P0;
   Triangle->P1 = P1;
   Triangle->P2 = P2;
   Triangle->Color = Color;
}

// TODO(hbr): Probably move to math
internal render_transform
operator*(render_transform A, render_transform B)
{
   render_transform Result = {};
   Result.Offset = A.Offset + B.Offset;
   Result.Rotation = CombineRotations2D(A.Rotation, B.Rotation);
   Result.Scale = Hadamard(A.Scale, B.Scale);
   
   return Result;
}

internal render_transform
Identity(void)
{
   render_transform Result = {};
   Result.Rotation = Rotation2DZero();
   Result.Scale = V2(1.0f, 1.0f);
   
   return Result;
}

internal v2
Project(render_transform *XForm, v2 Pos)
{
   v2 Result = Pos - XForm->Offset;
   RotateAround(Result, V2(0.0f, 0.0f), Rotation2DInverse(XForm->Rotation));
   Result = Hadamard(Result, XForm->Scale);
   
   return Result;
}

internal v2
Unproject(render_transform *XForm, v2 Pos)
{
   v2 Result = Hadamard(Pos, V2(1.0f / XForm->Scale.X, 1.0f / XForm->Scale.Y));
   Result = RotateAround(Result, V2(0.0f, 0.0f), XForm->Rotation);
   Result += XForm->Offset;
   
   return Result;
}

internal void
BeginRenderGroup(render_group *Group, render_frame *Frame,
                 v2 CameraP, rotation_2d CameraRot, f32 CameraZoom,
                 f32 FrustumSize, f32 AspectRatio, s32 WindowWidth, s32 WindowHeight,
                 v4 ClearColor)
{
   Group->Frame = Frame;
   
   Group->WorldToCamera.Offset = CameraP;
   Group->WorldToCamera.Rotation = CameraRot;
   Group->WorldToCamera.Scale = V2(CameraZoom, CameraZoom);
   
   Group->CameraToClip.Offset = V2(0.0f, 0.0f);
   Group->CameraToClip.Rotation = Rotation2DZero();
   Group->CameraToClip.Scale = V2(1.0f / (0.5f * FrustumSize * AspectRatio),
                                  1.0f / (0.5f * FrustumSize));
   
   Group->ClipToScreen.Offset = V2(-1.0f, 1.0f);
   Group->ClipToScreen.Scale = 0.5f * V2(WindowWidth, -WindowHeight);
   Group->ClipToScreen.Rotation = Rotation2DZero();
   
   Frame->ClearColor = ClearColor;
   Frame->Proj = Group->CameraToClip * Group->WorldToCamera;
   Frame->WindowDim = V2S32(WindowWidth, WindowHeight);
}
