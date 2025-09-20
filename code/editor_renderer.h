#ifndef EDITOR_RENDERER_H
#define EDITOR_RENDERER_H

struct mat3_col_major
{
 mat3 M;
};
struct mat3_row_major
{
 mat3 M;
};
internal mat3_col_major ColMajor3x3From3x3(mat3 M);
internal mat3_row_major RowMajor3x3From3x3(mat3 M);

enum render_primitive_type
{
 Primitive_Triangles,
 Primitive_TriangleStrip,
};

struct vertex_array
{
 u32 VertexCount;
 v2 *Vertices;
 render_primitive_type Primitive;
};

enum render_command_type
{
 RenderCommand_VertexArray,
};

struct render_line
{
 u32 VertexCount;
 v2 *Vertices;
 render_primitive_type Primitive;
 rgba Color;
 mat3_row_major Model;
 f32 ZOffset;
};

struct render_circle
{
 f32 Z;
 mat3_col_major Model;
 f32 RadiusProper;
 rgba Color;
 rgba OutlineColor;
};

struct render_texture_handle
{
 u32 U32[1];
};
internal render_texture_handle TextureHandleZero(void);
internal b32 TextureHandleMatch(render_texture_handle T1, render_texture_handle T2);
internal render_texture_handle TextureHandleFromIndex(u32 Index);
internal u32 TextureIndexFromHandle(render_texture_handle Handle);

struct render_image
{
 f32 Z;
 mat3_col_major Model;
 render_texture_handle TextureHandle;
};

struct render_vertex
{
 v2 P;
 f32 Z;
 rgba Color;
};

struct render_frame
{
 arena *Arena;
 
 u32 LineCount;
 render_line *Lines;
 u32 MaxLineCount;
 
 u32 CircleCount;
 render_circle *Circles;
 u32 MaxCircleCount;
 
 u32 ImageCount;
 render_image *Images;
 u32 MaxImageCount;
 
 u32 VertexCount;
 render_vertex *Vertices;
 u32 MaxVertexCount;
 
 v2u WindowDim;
 
 mat3 Proj;
 rgba ClearColor;
 
 b32 PolygonModeIsWireFrame;
};

struct render_group
{
 render_frame *Frame;
 
 mat3_inv ProjXForm;
 mat3 ModelXForm;
 f32 ZOffset;
 
 f32 CameraZoom;
 f32 AspectRatio;
};
internal render_group BeginRenderGroup(render_frame *Frame, v2 CameraP, rotation2d CameraRot, f32 CameraZoom, rgba ClearColor);
internal void PushVertexArray(render_group *Group, v2 *Vertices, u32 VertexCount, render_primitive_type Primitive, rgba Color, f32 ZOffset);
internal void PushCircle(render_group *Group, v2 P, f32 Radius, rgba Color, f32 ZOffset, f32 OutlineThickness = 0, rgba OutlineColor = RGBA(0, 0, 0, 0));
internal void PushRectangle(render_group *Group, v2 P, v2 Size, rotation2d Rotation, rgba Color, f32 ZOffset);
internal void PushLine(render_group *Group, v2 BeginPoint, v2 EndPoint, f32 LineWidth, rgba Color, f32 ZOffset);
internal void PushTriangle(render_group *Group, v2 P0, v2 P1, v2 P2, rgba Color, f32 ZOffset);
internal void PushImage(render_group *Group, scale2d Dim, render_texture_handle TextureHandle);
internal f32 ClipSpaceLengthToWorldSpace(render_group *RenderGroup, f32 Clip);
internal void SetTransform(render_group *RenderGroup, mat3 Model, f32 ZOffset);
internal void ResetTransform(render_group *RenderGroup);
internal void SetPolygonMode(render_group *RenderGroup, b32 WireFrame);

enum renderer_transfer_op_type
{
 RendererTransferOp_Texture,
 RendererTransferOp_Buffer,
};
enum renderer_transfer_op_state
{
 RendererOp_Empty, // NOTE(hbr): use for cancellation as well
 RendererOp_PendingLoad,
 RendererOp_ReadyToTransfer,
};
struct renderer_transfer_op
{
 renderer_transfer_op_state State;
 volatile renderer_transfer_op_type Type;
 u64 SavedAllocateOffset;
 u64 PreviousAllocateOffset;
 
 union {
  struct {
   render_texture_handle TextureHandle;
   u32 Width;
   u32 Height;
   char *Pixels;
  };
  struct {
   u32 BufferIndex;
   void *Buffer;
   u64 BufferSize;
  };
 };
};
struct renderer_transfer_queue
{
#define MAX_RENDERER_TRANFER_QUEUE_COUNT 128
 renderer_transfer_op Ops[MAX_RENDERER_TRANFER_QUEUE_COUNT];
 u32 FirstOpIndex;
 u32 OpCount;
 
 char *TransferMemory;
 u64 AllocateOffset;
 u64 FreeOffset;
 u64 TransferMemorySize;
};
internal renderer_transfer_op *PushTextureTransfer(renderer_transfer_queue *Queue, u32 TextureWidth, u32 TextureHeight, u64 SizeInBytes, render_texture_handle TextureHandle);
internal void PopTextureTransfer(renderer_transfer_queue *Queue, renderer_transfer_op *Op);

struct platform_renderer_limits
{
 u32 MaxTextureCount;
 u32 MaxBufferCount;
};

struct renderer_header
{
 void *Platform; // NOTE(hbr): Platform layer for a given renderer can put here anything
};
struct renderer
{
 renderer_header Header;
};

struct renderer_memory
{
 renderer_transfer_queue RendererQueue;
 
 platform_renderer_limits Limits;
 
 render_line *LineBuffer;
 u32 MaxLineCount;
 
 render_circle *CircleBuffer;
 u32 MaxCircleCount;
 
 render_image *ImageBuffer;
 u32 MaxImageCount;
 
 render_vertex *VertexBuffer;
 u32 MaxVertexCount;
 
 platform_api PlatformAPI;
 
 profiler *Profiler;
 
 imgui_NewFrame *ImGuiNewFrame;
 imgui_Render *ImGuiRender;
};

#define RENDERER_BEGIN_FRAME(Name) render_frame *Name(renderer *Renderer, renderer_memory *Memory, v2u WindowDim)
typedef RENDERER_BEGIN_FRAME(renderer_begin_frame);

#define RENDERER_END_FRAME(Name) void Name(renderer *Renderer, renderer_memory *Memory, render_frame *Frame)
typedef RENDERER_END_FRAME(renderer_end_frame);

#define RENDERER_ON_CODE_RELOAD(Name) void Name(renderer_memory *Memory)
typedef RENDERER_ON_CODE_RELOAD(renderer_on_code_reload);

#endif //EDITOR_RENDERER_H
