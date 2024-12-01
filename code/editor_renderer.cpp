internal render_command *
PushRenderCommand(render_group *Group, render_command_type Type, mat3 Model, f32 ZOffset)
{
 render_frame *Frame = Group->Frame;
 Assert(Frame->CommandCount < Frame->MaxCommandCount);
 render_command *Result = Frame->Commands + Frame->CommandCount;
 ++Frame->CommandCount;
 Result->Type = Type;
 Result->ModelXForm = Group->ModelXForm * Model;
 Result->ZOffset = Group->ZOffset + ZOffset;
 
 return Result;
}

internal void
PushVertexArray(render_group *Group,
                v2 *Vertices,
                u32 VertexCount,
                render_primitive_type Primitive,
                v4 Color,
                f32 ZOffset)
{
 render_command *Command = PushRenderCommand(Group, RenderCommand_VertexArray, Identity3x3(), ZOffset);
 
 render_command_vertex_array *Array = &Command->VertexArray;
 Array->Vertices = Vertices;
 Array->VertexCount = VertexCount;
 Array->Primitive = Primitive;
 Array->Color = Color;
}

internal void
PushCircle(render_group *Group,
           v2 P, v2 Rotation, v2 Scale,
           f32 Radius, v4 Color,
           f32 ZOffset,
           f32 OutlineThickness = 0,
           v4 OutlineColor = V4(0, 0, 0, 0))
{
 render_frame *Frame = Group->Frame;
 if (Frame->CircleCount < Frame->MaxCircleCount)
 {
  render_circle *Data = Frame->Circles + Frame->CircleCount++;
  
  f32 TotalRadius = Radius + OutlineThickness;
  f32 RadiusProper = Radius / TotalRadius;
  
  mat3 Model = ModelTransform(P, Rotation, Scale);
  Model = Scale3x3(Model, TotalRadius);
  Model = Transpose3x3(Model);
  
  Data->Z = ZOffset;
  Data->Model = Model;
  Data->RadiusProper = RadiusProper;
  Data->Color = Color;
  Data->OutlineColor = OutlineColor;
 }
}

internal void
PushRectangle(render_group *Group,
              v2 P, v2 Size, v2 Rotation,
              v4 Color, f32 ZOffset)
{
 mat3 Model = ModelTransform(P, Rotation, 0.5f * Size);
 render_command *Command = PushRenderCommand(Group, RenderCommand_Rectangle, Model, ZOffset);
 
 render_command_rectangle *Rectangle = &Command->Rectangle;
 Rectangle->Color = Color;
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
 render_command *Command = PushRenderCommand(Group, RenderCommand_Triangle, Identity3x3(), ZOffset);
 
 render_command_triangle *Triangle = &Command->Triangle;
 Triangle->P0 = P0;
 Triangle->P1 = P1;
 Triangle->P2 = P2;
 Triangle->Color = Color;
}

internal void
PushImage(render_group *Group, v2 Dim, u32 TextureIndex)
{
 mat3 Model = ModelTransform(V2(0, 0), Rotation2DZero(), Dim);
 render_command *Command = PushRenderCommand(Group, RenderCommand_Image, Model, 0.0f);
 
 render_command_image *Image = &Command->Image;
 Image->TextureIndex = TextureIndex;
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
 mat3_inv CameraT = CameraTransform(CameraP, CameraRot, CameraZoom);
 mat3_inv ClipT = ClipTransform(AspectRatio);
 
 Result.ProjXForm.Forward = ClipT.Forward * CameraT.Forward;
 Result.ProjXForm.Inverse = CameraT.Inverse * ClipT.Inverse;
 Result.ModelXForm = Identity3x3();
 
 Result.CollisionTolerance = CollisionToleranceClip / CameraZoom;
 Result.RotationRadius = CollisionToleranceClip / CameraZoom;
 
 Frame->Proj = Result.ProjXForm.Forward;
 Frame->ClearColor = ClearColor;
 
 return Result;
}

internal renderer_transfer_op *
PushTextureTransfer(renderer_transfer_queue *Queue, u64 RequestSize)
{
 renderer_transfer_op *Result = 0;
 
 u64 NewUsed = Queue->TransferMemoryUsed + RequestSize;
 if ((Queue->OpCount < ArrayCount(Queue->Ops)) && (NewUsed <= Queue->TransferMemorySize))
 {
  Result = Queue->Ops + Queue->OpCount++;
  Result->Pixels = Queue->TransferMemory + Queue->TransferMemoryUsed;
  Queue->TransferMemoryUsed = NewUsed;
 }
 
 return Result;
}

internal void
PopTextureTransfer(renderer_transfer_queue *Queue, renderer_transfer_op *Op)
{
 Assert(Queue->OpCount > 0);
 --Queue->OpCount;
 Assert(Queue->Ops + Queue->OpCount == Op);
 Queue->TransferMemoryUsed = Op->Pixels - Queue->TransferMemory;
}

internal void
SetTransform(render_group *RenderGroup, mat3 Model, f32 ZOffset)
{
 RenderGroup->ModelXForm = Model;
 RenderGroup->ZOffset = ZOffset;
}

internal void
ResetTransform(render_group *RenderGroup)
{
 RenderGroup->ModelXForm = Identity3x3();
 RenderGroup->ZOffset = 0;
}