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
   RenderCommand_Clear,
   RenderCommand_VertexArray,
   RenderCommand_Circle,
   RenderCommand_Rectangle,
   RenderCommand_Triangle,
};

global u64 GlobalTransformCount;
global sf::Transform GlobalTransformBuffer[4096];

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
      render_command_clear Clear;
      render_command_circle Circle;
      render_command_rectangle Rectangle;
      render_command_triangle Triangle;
   };
   sf::Transform *Transform;
};

struct render_commands
{
   u64 CommandCount;
   render_command *Commands;
   u64 MaxCommandCount;
};

struct sfml_renderer
{
   render_commands Commands;
   
   sf::RenderWindow *Window;
   
   render_command CommandBuffer[4096];
};

internal sfml_renderer *SFMLInit(arena *Arena, sf::RenderWindow *Window);
internal render_commands *SFMLBeginFrame(sfml_renderer *SFML);
internal void SFMLEndFrame(sfml_renderer *SFML, render_commands *Commands);

#endif //EDITOR_RENDERER_H
