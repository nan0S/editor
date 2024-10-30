internal int
RenderCommandCmp(render_command *A, render_command *B)
{
 return Cmp(A->ZOffset, B->ZOffset);
}

internal render_command *
PushRenderCommand(render_group *Group, render_command_type Type, f32 ZOffset)
{
 render_frame *Frame = Group->Frame;
 Assert(Frame->CommandCount < Frame->MaxCommandCount);
 render_command *Result = Frame->Commands + Frame->CommandCount;
 ++Frame->CommandCount;
 Result->Type = Type;
 Result->ZOffset = Group->ZOffset + ZOffset;
 
 return Result;
}

internal void
PushVertexArray(render_group *Group,
                vertex *Vertices,
                u64 VertexCount,
                render_primitive_type Primitive,
                v4 Color,
                f32 ZOffset)
{
 render_command *Command = PushRenderCommand(Group, RenderCommand_VertexArray, ZOffset);
 
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
           f32 ZOffset,
           f32 OutlineThickness = 0,
           v4 OutlineColor = V4(0, 0, 0, 0))
{
 render_command *Command = PushRenderCommand(Group, RenderCommand_Circle, ZOffset);
 
 render_command_circle *Circle = &Command->Circle;
 Circle->Pos = Transform(Group->ModelXForm, Position);
 Circle->Radius = TransformLength(Group->ModelXForm, V2(Radius, 0)).X;
 Circle->Color = Color;
 Circle->OutlineThickness = OutlineThickness;
 Circle->OutlineColor = OutlineColor;
}

internal void
PushRectangle(render_group *Group,
              v2 Position, v2 Size, v2 Rotation,
              v4 Color, f32 ZOffset)
{
 render_command *Command = PushRenderCommand(Group, RenderCommand_Rectangle, ZOffset);
 
 render_command_rectangle *Rectangle = &Command->Rectangle;
 Rectangle->Pos = Transform(Group->ModelXForm, Position);
 Rectangle->Size = TransformLength(Group->ModelXForm, Size);
 Rectangle->Rotation = Rotation;
 Rectangle->Color = Color;
}

internal void
PushSquare(render_group *Group, v2 Position, f32 Side, v4 Color, f32 ZOffset)
{
 PushRectangle(Group, Position, V2(Side, Side), Rotation2DZero(), Color,  ZOffset);
}

internal void
PushLine(render_group *Group,
         v2 BeginPoint, v2 EndPoint,
         f32 LineWidth, v4 Color,
         f32 ZOffset)
{
 v2 Position = 0.5f * (BeginPoint + EndPoint);
 v2 Line = EndPoint - BeginPoint;
 f32 Length = Norm(Line);
 v2 Size = V2(LineWidth, Length);
 // NOTE(hbr): Rotate 90 degrees clockwise, because our 0 degree rotation
 // corresponds to -90 degrees rotation in the real world
 v2 Rotation = Rotate90DegreesClockwise(Rotation2DFromVector(Line));
 PushRectangle(Group, Position, Size, Rotation, Color, ZOffset);
}

internal void
PushTriangle(render_group *Group,
             v2 P0, v2 P1, v2 P2,
             v4 Color, f32 ZOffset)
{
 render_command *Command = PushRenderCommand(Group, RenderCommand_Triangle, ZOffset);
 
 render_command_triangle *Triangle = &Command->Triangle;
 Triangle->P0 = Transform(Group->ModelXForm, P0);
 Triangle->P1 = Transform(Group->ModelXForm, P1);
 Triangle->P2 = Transform(Group->ModelXForm, P2);
 Triangle->Color = Color;
}

internal void
PushImage(render_group *Group, v2 Dim, u64 Width, u64 Height, char *Pixels)
{
 render_command *Command = PushRenderCommand(Group, RenderCommand_Image, 0.0f);
 
 render_command_image *Image = &Command->Image;
 Image->Width = Width;
 Image->Height = Height;
 Image->Pixels = Pixels;
 Image->Dim = Dim;
 Image->P = Group->ModelXForm.Offset;
 Image->Rotation = Group->ModelXForm.Rotation;
 Image->Scale = Group->ModelXForm.Scale;
}

internal transform_inv
MakeRenderTransform(v2 P, v2 Rotation, v2 Scale)
{
 transform_inv Result = {};
 
 transform A = {};
 A.Scale = V2(1.0f / Scale.X, 1.0f / Scale.Y);
 A.Rotation = Rotation2DInverse(Rotation);
 A.Offset = -RotateAround(P, V2(0, 0), A.Rotation);
 A.Offset = Hadamard(A.Offset, A.Scale);
 
 transform B = {};
 B.Scale = Scale;
 B.Rotation = Rotation;
 B.Offset = P;
 
 Result.Forward = A;
 Result.Inverse = B;
 
 return Result;
}

internal render_group
BeginRenderGroup(render_frame *Frame,
                 v2 CameraP, v2 CameraRot, f32 CameraZoom,
                 v4 ClearColor, f32 CollisionToleranceClip,
                 f32 RotationRadiusClip)
{
 render_group Result = {};
 
 Result.Frame = Frame;
 
 v2u WindowDim = Frame->WindowDim;
 f32 AspectRatio = Cast(f32)WindowDim.X / WindowDim.Y;
 
 Result.WorldToCamera = MakeRenderTransform(CameraP, CameraRot, V2(1.0f / CameraZoom, 1.0f / CameraZoom));
 Result.CameraToClip = MakeRenderTransform(V2(0, 0), Rotation2DZero(), V2(AspectRatio, 1.0f));
 Result.ClipToScreen = MakeRenderTransform(-V2(1.0f, -1.0f),
                                           Rotation2DZero(),
                                           V2(2.0f / WindowDim.X, -2.0f / WindowDim.Y));
 
 Result.ProjXForm.Forward = Result.CameraToClip.Forward * Result.WorldToCamera.Forward;
 Result.ProjXForm.Inverse = Result.WorldToCamera.Inverse * Result.CameraToClip.Inverse;
 
 Frame->ClearColor = ClearColor;
 Frame->Proj = Result.CameraToClip.Forward * Result.WorldToCamera.Forward;
 
 Result.ModelXForm = Identity();
 
 Result.CollisionTolerance = UnprojectLength(&Result.WorldToCamera, V2(CollisionToleranceClip, 0)).X;
 Result.RotationRadius = UnprojectLength(&Result.WorldToCamera, V2(RotationRadiusClip, 0)).X;
 
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