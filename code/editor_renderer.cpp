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

internal v2
Transform(transform A, v2 P)
{
   v2 Result = Hadamard(P, A.Scale);
   Result = RotateAround(Result, V2(0, 0), A.Rotation);
   Result = Result + A.Offset;
   
   return Result;
}

internal v2
TransformLength(transform A, v2 Length)
{
   v2 Result = Hadamard(Length, A.Scale);
   return Result;
}

internal v2
ProjectLength(render_transform *XForm, v2 Length)
{
   v2 Result = TransformLength(XForm->Forward, Length);
   return Result;
}

internal v2
UnprojectLength(render_transform *XForm, v2 Length)
{
   v2 Result = TransformLength(XForm->Inverse, Length);
   return Result;
}

internal v2
Project(render_transform *XForm, v2 P)
{
   v2 Result = Transform(XForm->Forward, P);
   return Result;
}

internal v2
Unproject(render_transform *XForm, v2 P)
{
   v2 Result = Transform(XForm->Inverse, P);
   return Result;
}

internal sf::Transform
RenderTransformToSFMLTransform(transform A)
{
   sf::Transform Result = {};
#if 0
   Result= sf::Transform()
      .scale(1.0f / XForm->Scale.X, 1.0f / XForm->Scale.Y)
      .rotate(Rotation2DToDegrees(Rotation2DInverse(XForm->Rotation)))
      .translate(-XForm->Offset.X, -XForm->Offset.Y);
#endif
   Result = sf::Transform()
      
      
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
                v4 Color)
{
   render_command *Command = PushRenderCommand(Group, RenderCommand_VertexArray);
   
   render_command_vertex_array *Array = &Command->VertexArray;
   Array->Vertices = Vertices;
   Array->VertexCount = VertexCount;
   Array->Primitive = Primitive;
   Array->Color = Color;
   Array->ModelXForm = Group->ModelXForm;
}

internal void
PushCircle(render_group *Group,
           v2 Position, f32 Radius, v4 Color,
           f32 OutlineThickness = 0.0f,
           v4 OutlineColor = V4(0.0f, 0.0f, 0.0f, 0.0f))
{
   render_command *Command = PushRenderCommand(Group, RenderCommand_Circle);
   
   render_command_circle *Circle = &Command->Circle;
   Circle->Pos = Transform(Group->ModelXForm, Position);
   Circle->Radius = TransformLength(Group->ModelXForm, V2(Radius, 0)).X;
   Circle->Color = Color;
   Circle->OutlineThickness = OutlineThickness;
   Circle->OutlineColor = OutlineColor;
}

internal void
PushRectangle(render_group *Group,
              v2 Position, v2 Size, rotation_2d Rotation,
              v4 Color)
{
   render_command *Command = PushRenderCommand(Group, RenderCommand_Rectangle);
   
   render_command_rectangle *Rectangle = &Command->Rectangle;
   Rectangle->Pos = Transform(Group->ModelXForm, Position);
   Rectangle->Size = TransformLength(Group->ModelXForm, Size);
   Rectangle->Rotation = Rotation;
   Rectangle->Color = Color;
}

internal void
PushSquare(render_group *Group, v2 Position, f32 Side, v4 Color)
{
   PushRectangle(Group, Position, V2(Side, Side), Rotation2DZero(), Color);
}

internal void
PushLine(render_group *Group,
         v2 BeginPoint, v2 EndPoint,
         f32 LineWidth, v4 Color)
{
   v2 Position = 0.5f * (BeginPoint + EndPoint);
   v2 Line = EndPoint - BeginPoint;
   f32 Length = Norm(Line);
   v2 Size = V2(LineWidth, Length);
   // NOTE(hbr): Rotate 90 degrees clockwise, because our 0 degree rotation
   // corresponds to -90 degrees rotation in the real world
   rotation_2d Rotation = Rotate90DegreesClockwise(Rotation2DFromVector(Line));
   PushRectangle(Group, Position, Size, Rotation, Color);
}

internal void
PushTriangle(render_group *Group,
             v2 P0, v2 P1, v2 P2,
             v4 Color)
{
   render_command *Command = PushRenderCommand(Group, RenderCommand_Triangle);
   
   render_command_triangle *Triangle = &Command->Triangle;
   Triangle->P0 = Transform(Group->ModelXForm, P0);
   Triangle->P1 = Transform(Group->ModelXForm, P1);
   Triangle->P2 = Transform(Group->ModelXForm, P2);
   Triangle->Color = Color;
}

// TODO(hbr): Probably move to math
internal transform
operator*(transform T2, transform T1)
{
   transform Result = {};
   Result.Scale = Hadamard(T2.Scale, T1.Scale);
   Result.Rotation = CombineRotations2D(T2.Rotation, T1.Rotation);
   Result.Offset = RotateAround(Hadamard(T2.Scale, T1.Offset), V2(0, 0), T2.Rotation) + T2.Offset;
   
   return Result;
}

internal transform
Identity(void)
{
   transform Result = {};
   Result.Rotation = Rotation2DZero();
   Result.Scale = V2(1.0f, 1.0f);
   
   return Result;
}

internal render_transform
MakeRenderTransform(v2 P, rotation_2d Rotation, v2 Scale)
{
   render_transform Result = {};
   
   transform A = {};
   A.Scale = V2(1.0f / Scale.X, 1.0f / Scale.Y);
   A.Rotation = Rotation2DInverse(Rotation);
   A.Offset = -RotateAround(P, V2(0, 0), A.Rotation);
   
   transform B = {};
   B.Scale = Scale;
   B.Rotation = Rotation;
   B.Offset = P;
   
   Result.Forward = A;
   Result.Inverse = B;
   
   return Result;
}

struct m3x3
{
   f32 M[3][3];
};

struct m3x3_inv
{
   m3x3 Forward;
   m3x3 Inverse;
};

internal render_group
BeginRenderGroup(render_frame *Frame,
                 v2 CameraP, rotation_2d CameraRot, f32 CameraZoom,
                 v4 ClearColor)
{
   render_group Result = {};
   
   Result.Frame = Frame;
   
   v2s WindowDim = Frame->WindowDim;
   f32 AspectRatio = Cast(f32)WindowDim.X / WindowDim.Y;
   
   Result.WorldToCamera = MakeRenderTransform(CameraP, CameraRot, V2(1.0f / CameraZoom, 1.0f / CameraZoom));
   Result.CameraToClip = MakeRenderTransform(V2(0, 0), Rotation2DZero(), V2(AspectRatio, 1.0f));
   Result.ClipToScreen = MakeRenderTransform(-V2(1.0f, -1.0f),
                                             Rotation2DZero(),
                                             2.0f * V2(1.0f / WindowDim.X, -1.0f / WindowDim.Y));
   
   Frame->ClearColor = ClearColor;
   Frame->Proj = Result.CameraToClip.Forward * Result.WorldToCamera.Forward;
   
   Result.ModelXForm = Identity();
   
   return Result;
}

internal void
SetModelTransform(render_group *Group, transform Model, f32 ZOffset)
{
   Group->ModelXForm = Model;
   Group->ZOffset = ZOffset;
}

internal void
ResetModelTransform(render_group *Group)
{
   Group->ModelXForm = Identity();
   Group->ZOffset = 0.0f;
}