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
   u64 VertexCount;
   vertex *Vertices;
   render_primitive_type Primitive;
};

enum render_command_type
{
   RenderCommand_VertexArray,
   RenderCommand_Circle,
   RenderCommand_Rectangle,
   RenderCommand_Triangle,
};

struct render_command_clear
{
   v4 Color;
};

struct render_command_vertex_array
{
   u64 VertexCount;
   vertex *Vertices;
   render_primitive_type Primitive;
   v4 Color;
   transform ModelXForm;
};

struct render_command_circle
{
   v2 Pos;
   f32 Radius;
   v4 Color;
   f32 OutlineThickness;
   v4 OutlineColor;
};

struct render_command_rectangle
{
   v2 Pos;
   v2 Size;
   rotation_2d Rotation;
   v4 Color;
};

struct render_command_triangle
{
   v2 P0;
   v2 P1;
   v2 P2;
   v4 Color;
};

struct render_command
{
   render_command_type Type;
   union {
      render_command_vertex_array VertexArray;
      render_command_circle Circle;
      render_command_rectangle Rectangle;
      render_command_triangle Triangle;
   };
   f32 ZOffset;
};

struct render_frame
{
   u64 CommandCount;
   render_command *Commands;
   u64 MaxCommandCount;
   
   v2u WindowDim;
   
   transform Proj;
   v4 ClearColor;
};

struct render_group
{
   render_frame *Frame;
   
   transform_inv WorldToCamera;
   transform_inv CameraToClip;
   transform_inv ClipToScreen;
   
   transform_inv ProjXForm;
   
   transform ModelXForm;
   f32 ZOffset;
   
   f32 CollisionTolerance;
};

#endif //EDITOR_RENDERER_H
