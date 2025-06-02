internal mat3_col_major
ColMajor3x3From3x3(mat3 M)
{
 mat3_col_major C = {};
 C.M = Transpose3x3(M);
 return C;
}

internal mat3_row_major
RowMajor3x3From3x3(mat3 M)
{
 mat3_row_major R = {};
 R.M = M;
 return R;
}

inline internal render_texture_handle
TextureHandleZero(void)
{
 render_texture_handle Result = {};
 return Result;
}

inline internal b32
TextureHandleMatch(render_texture_handle T1, render_texture_handle T2)
{
 b32 Result = (T1.U32[0] == T2.U32[1]);
 return Result;
}

inline internal render_texture_handle
TextureHandleFromIndex(u32 Index)
{
 render_texture_handle Result = {};
 Result.U32[0] = Index;
 return Result;
}

inline internal u32
TextureIndexFromHandle(render_texture_handle Handle)
{
 u32 Index = Handle.U32[0];
 return Index;
}

internal void
PushVertexArray(render_group *Group,
                v2 *Vertices,
                u32 VertexCount,
                render_primitive_type Primitive,
                v4 Color,
                f32 ZOffset)
{
 ProfileFunctionBegin();
 
 render_frame *Frame = Group->Frame;
 if (Frame->LineCount < Frame->MaxLineCount)
 {
  render_line *Line = Frame->Lines + Frame->LineCount++;
  Line->Vertices = Vertices;
  Line->VertexCount = VertexCount;
  Line->Primitive = Primitive;
  Line->Color = Color;
  Line->Model = RowMajor3x3From3x3(Group->ModelXForm);
  Line->ZOffset = ZOffset + Group->ZOffset;
 }
 
 ProfileEnd();
}

internal void
PushCircle(render_group *Group,
           v2 P,
           f32 Radius, v4 Color,
           f32 ZOffset,
           f32 OutlineThickness = 0,
           v4 OutlineColor = V4(0, 0, 0, 0))
{
 ProfileFunctionBegin();
 
 render_frame *Frame = Group->Frame;
 if (Frame->CircleCount < Frame->MaxCircleCount)
 {
  render_circle *Circle = Frame->Circles + Frame->CircleCount++;
  
  f32 TotalRadius = Radius + OutlineThickness;
  f32 RadiusProper = Radius / TotalRadius;
  
  mat3 Model = Identity3x3();
  Model = Translate3x3(Model, P);
  Model = Scale3x3(Model, TotalRadius);
  Model = Group->ModelXForm * Model;
  
  Circle->Z = ZOffset;
  Circle->Model = ColMajor3x3From3x3(Model);
  Circle->RadiusProper = RadiusProper;
  Circle->Color = Color;
  Circle->OutlineColor = OutlineColor;
 }
 
 ProfileEnd();
}

internal void
PushRectangle(render_group *Group,
              v2 P, v2 Size, v2 Rotation,
              v4 Color, f32 ZOffset)
{
 ProfileFunctionBegin();
 
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
 
 ProfileEnd();
}

internal void
PushLine(render_group *Group,
         v2 BeginPoint, v2 EndPoint,
         f32 LineWidth, v4 Color,
         f32 ZOffset)
{
 ProfileFunctionBegin();
 
 v2 Position = 0.5f * (BeginPoint + EndPoint);
 v2 Line = EndPoint - BeginPoint;
 f32 Length = Norm(Line);
 v2 Size = V2(Length, LineWidth);
 v2 Rotation = Rotation2DFromVector(Line);
 PushRectangle(Group, Position, Size, Rotation, Color, ZOffset);
 
 ProfileEnd();
}

internal void
PushTriangle(render_group *Group,
             v2 P0, v2 P1, v2 P2,
             v4 Color, f32 ZOffset)
{
 ProfileFunctionBegin();
 
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
 
 ProfileEnd();
}

internal void
PushImage(render_group *Group, v2 Dim, render_texture_handle TextureHandle)
{
 render_frame *Frame = Group->Frame;
 if (Frame->ImageCount < Frame->MaxImageCount)
 {
  render_image *RenderImage = Frame->Images + Frame->ImageCount++;
  
  mat3 Model = ModelTransform(V2(0, 0), Rotation2DZero(), Dim);
  Model = Group->ModelXForm * Model;
  
  RenderImage->Model = ColMajor3x3From3x3(Model);
  RenderImage->TextureHandle = TextureHandle;
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
 Result.AspectRatio = AspectRatio;
 
 Frame->Proj = Result.ProjXForm.Forward;
 Frame->ClearColor = ClearColor;
 
 return Result;
}

internal renderer_transfer_op *
PushTextureTransfer(renderer_transfer_queue *Queue,
                    u32 TextureWidth, u32 TextureHeight, u32 TextureChannels,
                    render_texture_handle TextureHandle)
{
 renderer_transfer_op *Op = 0;
 if (Queue->OpCount < MAX_RENDERER_TRANFER_QUEUE_COUNT)
 {
  u64 PreviousAllocateOffset = Queue->AllocateOffset;
  u64 RequestSize = Cast(u64)TextureWidth * TextureHeight * TextureChannels;
  
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
   Op = Queue->Ops + OpIndex;
   ++Queue->OpCount;
   
   Op->Pixels = Queue->TransferMemory + Queue->AllocateOffset;
   Op->State = RendererOp_PendingLoad;
   Op->PreviousAllocateOffset = PreviousAllocateOffset;
   Queue->AllocateOffset += RequestSize;
   Op->SavedAllocateOffset = Queue->AllocateOffset;
   Op->Width = TextureWidth;
   Op->Height = TextureHeight;
   Op->TextureHandle = TextureHandle;
  }
  else
  {
   Queue->AllocateOffset = PreviousAllocateOffset;
  }
 }
 
 return Op;
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
 ProfileFunctionBegin();
 
 RenderGroup->ModelXForm = Model;
 RenderGroup->ZOffset = ZOffset;
 
 ProfileEnd();
}

internal void
ResetTransform(render_group *RenderGroup)
{
 ProfileFunctionBegin();
 
 RenderGroup->ModelXForm = Identity3x3();
 RenderGroup->ZOffset = 0;
 
 ProfileEnd();
}