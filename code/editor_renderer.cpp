
internal void
PushVertexArray(render_group *Group,
                v2 *Vertices,
                u32 VertexCount,
                render_primitive_type Primitive,
                v4 Color,
                f32 ZOffset)
{
 render_frame *Frame = Group->Frame;
 if (Frame->LineCount < Frame->MaxLineCount)
 {
  render_line *Line = Frame->Lines + Frame->LineCount++;
  Line->Vertices = Vertices;
  Line->VertexCount = VertexCount;
  Line->Primitive = Primitive;
  Line->Color = Color;
  Line->Model = Group->ModelXForm;
  Line->ZOffset = ZOffset + Group->ZOffset;
 }
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
 render_frame *Frame = Group->Frame;
 if (Frame->VertexCount + 6 < Frame->MaxVertexCount)
 {
  render_vertex *V = Frame->Vertices + Frame->VertexCount;
  
  v2 HalfSize = 0.5f * Size;
  
  v2 Corner00 = V2(P.X - HalfSize.X, P.Y - HalfSize.Y);
  v2 Corner10 = V2(P.X + HalfSize.X, P.Y - HalfSize.Y);
  v2 Corner11 = V2(P.X + HalfSize.X, P.Y + HalfSize.Y);
  v2 Corner01 = V2(P.X - HalfSize.X, P.Y + HalfSize.Y);
  
  // TODO(hbr): Map those corners through matrix
  // TODO(hbr): I don't know, is this ok???
  Corner00 = RotateAround(Corner00, P, Rotation);
  Corner01 = RotateAround(Corner01, P, Rotation);
  Corner11 = RotateAround(Corner11, P, Rotation);
  Corner10 = RotateAround(Corner10, P, Rotation);
  
  f32 Z = ZOffset + Group->ZOffset;
  
  V[0].P = Corner00;
  V[0].Z = Z;
  V[0].Color = Color;
  V[1].P = Corner10;
  V[1].Z = Z;
  V[1].Color = Color;
  V[2].P = Corner11;
  V[2].Z = Z;
  V[2].Color = Color;
  
  V[3].P = Corner00;
  V[3].Z = Z;
  V[3].Color = Color;
  V[4].P = Corner11;
  V[4].Z = Z;
  V[4].Color = Color;
  V[5].P = Corner01;
  V[5].Z = Z;
  V[5].Color = Color;
  
  Frame->VertexCount += 6;
 }
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
 v2 Size = V2(Length, LineWidth);
 v2 Rotation = Rotation2DFromVector(Line);
 PushRectangle(Group, Position, Size, Rotation, Color, ZOffset);
}

internal void
PushTriangle(render_group *Group,
             v2 P0, v2 P1, v2 P2,
             v4 Color, f32 ZOffset)
{
 render_frame *Frame = Group->Frame;
 if (Frame->VertexCount + 3 < Frame->MaxVertexCount)
 {
  render_vertex *V = Frame->Vertices + Frame->VertexCount;
  f32 Z = ZOffset + Group->ZOffset;
  
  V[0].P = P0;
  V[0].Z = ZOffset;
  V[0].Color = Color;
  
  V[1].P = P1; 
  V[1].Z = ZOffset;
  V[1].Color = Color;
  
  V[2].P = P2;
  V[2].Z = ZOffset;
  V[2].Color = Color;
  
  Frame->VertexCount += 3;
 }
}

internal void
PushImage(render_group *Group, v2 Dim, u32 TextureIndex)
{
 render_frame *Frame = Group->Frame;
 if (Frame->ImageCount < Frame->MaxImageCount)
 {
  render_image *RenderImage = Frame->Images + Frame->ImageCount++;
  
  mat3 Model = Group->ModelXForm * ModelTransform(V2(0, 0), Rotation2DZero(), Dim);
  Model = Transpose3x3(Model);
  
  RenderImage->Model = Model;
  RenderImage->TextureIndex = TextureIndex;
  RenderImage->Z = Group->ZOffset;
 }
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
 
 if (Queue->OpCount < MAX_RENDERER_TRANFER_QUEUE_COUNT)
 {
  u64 PreviousAllocateOffset = Queue->AllocateOffset;
  
  if (Queue->AllocateOffset >= Queue->FreeOffset &&
      (Queue->TransferMemorySize - Queue->AllocateOffset) < RequestSize &&
      RequestSize <= Queue->FreeOffset)
  {
   Queue->AllocateOffset = 0;
  }
  
  u64 FreeSpace;
  if (Queue->AllocateOffset >= Queue->FreeOffset)
  {
   FreeSpace = Queue->TransferMemorySize - Queue->AllocateOffset;;
  }
  else
  {
   FreeSpace = Queue->FreeOffset - Queue->AllocateOffset;
  }
  if ((Queue->AllocateOffset + FreeSpace == Queue->FreeOffset) &&
      (FreeSpace > 0))
  {
   --FreeSpace;
  }
  
  if (RequestSize <= FreeSpace)
  {
   u64 OpIndex = (Queue->FirstOpIndex + Queue->OpCount) % MAX_RENDERER_TRANFER_QUEUE_COUNT;
   Result = Queue->Ops + OpIndex;
   ++Queue->OpCount;
   Result->Pixels = Queue->TransferMemory + Queue->AllocateOffset;
   Result->State = RendererOp_PendingLoad;
   Result->PreviousAllocateOffset = PreviousAllocateOffset;
   Queue->AllocateOffset += RequestSize;
   Result->SavedAllocateOffset = Queue->AllocateOffset;
  }
  else
  {
   Queue->AllocateOffset = PreviousAllocateOffset;
  }
 }
 
 return Result;
}

internal void
PopTextureTransfer(renderer_transfer_queue *Queue, renderer_transfer_op *Op)
{
 --Queue->OpCount;
 Queue->AllocateOffset = Op->PreviousAllocateOffset;
 
 // TODO(hbr): remove
 if (Queue->OpCount == 0)
 {
  Assert(Queue->AllocateOffset == Queue->FreeOffset);
 }
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