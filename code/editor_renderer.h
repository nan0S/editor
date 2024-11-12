#ifndef EDITOR_RENDERER_H
#define EDITOR_RENDERER_H

struct vertex
{
 v2 Pos;
};

enum render_primitive_type
{
 Primitive_Triangles,
 Primitive_TriangleStrip,
};

struct vertex_array
{
 u32 VertexCount;
 vertex *Vertices;
 render_primitive_type Primitive;
};

enum render_command_type
{
 RenderCommand_VertexArray,
 RenderCommand_Circle,
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
 vertex *Vertices;
 render_primitive_type Primitive;
 v4 Color;
};

struct render_command_circle
{
 v4 Color;
 f32 OutlineThickness;
 v4 OutlineColor;
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
  render_command_circle Circle;
  render_command_rectangle Rectangle;
  render_command_triangle Triangle;
  render_command_image Image;
 };
 m3x3 ModelXForm;
 f32 ZOffset;
};

struct render_frame
{
 u32 CommandCount;
 render_command *Commands;
 u32 MaxCommandCount;
 
 v2u WindowDim;
 
 m3x3 Proj;
 v4 ClearColor;
};

struct render_group
{
 render_frame *Frame;
 
 m3x3_inv ProjXForm;
 m3x3 ModelXForm;
 f32 ZOffset;
 
 f32 CollisionTolerance;
 f32 RotationRadius;
};

struct texture_transfer_op
{
 u32 TextureIndex;
 u32 Width;
 u32 Height;
 char *Pixels;
};

struct texture_transfer_queue
{
 u32 OpCount;
 texture_transfer_op Ops[32];
 
 char *TransferMemory;
 u64 TransferMemoryUsed;
 u64 TransferMemorySize;
};

struct platform_renderer_limits
{
 u32 MaxCommandCount;
 u32 MaxTextureQueueMemorySize;
 u32 MaxTextureCount;
};

struct platform_renderer
{
 texture_transfer_queue TextureQueue;
};

#define RENDERER_BEGIN_FRAME(Name) render_frame *Name(platform_renderer *Renderer, v2u WindowDim)
#define RENDERER_END_FRAME(Name) void Name(platform_renderer *Renderer, render_frame *Frame)

#endif //EDITOR_RENDERER_H
