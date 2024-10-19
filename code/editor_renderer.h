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

struct render_transform
{
   v2 Offset;
   rotation_2d Rotation;
   v2 Scale;
};

internal render_transform
operator*(render_transform A, render_transform B)
{
   render_transform Result = {};
   Result.Offset = A.Offset + B.Offset;
   Result.Rotation = CombineRotations2D(A.Rotation, B.Rotation);
   Result.Scale = Hadamard(A.Scale, B.Scale);
   
   return Result;
}

internal render_transform
Identity(void)
{
   render_transform Result = {};
   Result.Rotation = Rotation2DZero();
   Result.Scale = V2(1.0f, 1.0f);
   
   return Result;
}

internal v2
Transform(render_transform *XForm, v2 Pos)
{
   v2 Result = Pos - XForm->Offset;
   RotateAround(Result, V2(0.0f, 0.0f), XForm->Rotation);
   Result = Hadamard(Result, XForm->Scale);
   
   return Result;
}

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
   render_transform XForm;
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
