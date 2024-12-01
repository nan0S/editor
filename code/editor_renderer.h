#ifndef EDITOR_RENDERER_H
#define EDITOR_RENDERER_H

enum render_primitive_type
{
 Primitive_Triangles,
 Primitive_TriangleStrip,
};

struct renderer_index;
struct vertex_array
{
 u32 VertexCount;
 v2 *Vertices;
 render_primitive_type Primitive;
};

enum render_command_type
{
 RenderCommand_VertexArray,
 RenderCommand_Rectangle,
 RenderCommand_Triangle,
 RenderCommand_Image,
};

struct render_command_clear
{
 v4 Color;
};

struct render_command_vertex_array
{
 u32 VertexCount;
 v2 *Vertices;
 render_primitive_type Primitive;
 v4 Color;
};

struct render_command_rectangle
{
 v4 Color;
};

struct render_command_triangle
{
 v2 P0;
 v2 P1;
 v2 P2;
 v4 Color;
};

struct render_command_image
{
 u32 TextureIndex;
};

struct render_command
{
 render_command_type Type;
 union {
  render_command_vertex_array VertexArray;
  render_command_rectangle Rectangle;
  render_command_triangle Triangle;
  render_command_image Image;
 };
 mat3 ModelXForm;
 f32 ZOffset;
};

struct render_circle
{
 f32 Z;
 mat3 Model;
 f32 RadiusProper;
 v4 Color;
 v4 OutlineColor;
};

struct render_image
{
 f32 Z;
 mat3 Model;
 u32 TextureIndex;
};

struct render_frame
{
 u32 CommandCount;
 render_command *Commands;
 u32 MaxCommandCount;
 
 u32 CircleCount;
 render_circle *Circles;
 u32 MaxCircleCount;
 
 u32 ImageCount;
 render_image *Images;
 u32 MaxImageCount;
 
 v2u WindowDim;
 
 mat3 Proj;
 v4 ClearColor;
};

struct render_group
{
 render_frame *Frame;
 
 mat3_inv ProjXForm;
 mat3 ModelXForm;
 f32 ZOffset;
 
 f32 CollisionTolerance;
 f32 RotationRadius;
};

enum renderer_transfer_op_type
{
 RendererTransferOp_Texture,
 RendererTransferOp_Buffer,
};
struct renderer_transfer_op
{
 renderer_transfer_op_type Type;
 union {
  struct {
   u32 TextureIndex;
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
 u32 OpCount;
 renderer_transfer_op Ops[32];
 
 char *TransferMemory;
 u64 TransferMemoryUsed;
 u64 TransferMemorySize;
};

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
 
 render_command *CommandBuffer;
 u32 MaxCommandCount;
 
 render_circle *CircleBuffer;
 u32 MaxCircleCount;
 
 render_image *ImageBuffer;
 u32 MaxImageCount;
 
 b32 RendererCodeReloaded;
 
 imgui_bindings ImGuiBindings;
 
 platform_api PlatformAPI;
};

#define RENDERER_BEGIN_FRAME(Name) render_frame *Name(renderer *Renderer, renderer_memory *Memory, v2u WindowDim)
typedef RENDERER_BEGIN_FRAME(renderer_begin_frame);

#define RENDERER_END_FRAME(Name) void Name(renderer *Renderer, renderer_memory *Memory, render_frame *Frame)
typedef RENDERER_END_FRAME(renderer_end_frame);

#endif //EDITOR_RENDERER_H
