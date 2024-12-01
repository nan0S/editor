#ifndef EDITOR_RENDERER_OPENGL_H
#define EDITOR_RENDERER_OPENGL_H

struct basic_program
{
 GLuint ProgramHandle;
 
 GLuint VertP_AttrLoc;
 GLuint VertColor_AttrLoc;
};
struct basic_program_vertex
{
 v2 P;
 v4 Color;
};

struct sample_program
{
 GLuint ProgramHandle;
 
 GLuint VertP_AttrLoc;
 GLuint VertUV_AttrLoc;
 GLuint Transform_UniformLoc;
 GLuint Color_UniformLoc;
};

struct perfect_circle_program
{
 GLuint ProgramHandle;
 
 union {
  GLuint Projection_UniformLoc;
  GLuint All[1];
 } Uniforms;
 
 union {
  struct {
   GLuint VertP_AttrLoc;
   GLuint VertZ_AttrLoc;
   GLuint VertModel0_AttrLoc;
   GLuint VertModel1_AttrLoc;
   GLuint VertModel2_AttrLoc;
   GLuint VertRadiusProper_AttrLoc;
   GLuint VertColor_AttrLoc;
   GLuint VertOutlineColor_AttrLoc;
  };
  GLuint All[8];
 } Attributes;
};
StaticAssert(SizeOf(MemberOf(perfect_circle_program, Attributes)) == SizeOf(MemberOf(perfect_circle_program, Attributes.All)),
             PerfectCircleProgram_AllAttributesArrayLengthMatchesIndividuallyDefinedAttributes);
StaticAssert(SizeOf(MemberOf(perfect_circle_program, Uniforms)) == SizeOf(MemberOf(perfect_circle_program, Uniforms.All)),
             PerfectCircleProgram_AllUniformsArrayLengthMatchesIndividuallyDefinedUniforms);

struct line_program
{
 GLuint ProgramHandle;
 
 union {
  struct {
   GLuint VertP_AttrLoc;
  };
  GLuint All[1];
 } Attributes;
 
 union {
  struct {
   GLuint Z_UniformLoc;
   GLuint Color_UniformLoc;
   GLuint Transform_UniformLoc;
  };
  GLuint All[3];
 } Uniforms;
};
StaticAssert(SizeOf(MemberOf(line_program, Attributes)) == SizeOf(MemberOf(line_program, Attributes.All)),
             LineProgram_AllAttributesArrayLengthMatchesIndividuallyDefinedAttributes);
StaticAssert(SizeOf(MemberOf(line_program, Uniforms)) == SizeOf(MemberOf(line_program, Uniforms.All)),
             LineProgram_AllUniformsArrayLengthMatchesIndividuallyDefinedUniforms);

struct image_program
{
 GLuint ProgramHandle;
 
 union {
  struct {
   GLuint VertPUV_AttrLoc;
  };
  GLuint All[1];
 } Attributes;
 
 union {
  struct {
   GLuint Z_UniformLoc;
   GLuint Model_UniformLoc;
   GLuint Projection_UniformLoc;
  };
  GLuint All[3];
 } Uniforms;
};
StaticAssert(SizeOf(MemberOf(image_program, Attributes)) == SizeOf(MemberOf(image_program, Attributes.All)),
             TexturedProgram_AllAttributesArrayLengthMatchesIndividuallyDefinedAttributes);
StaticAssert(SizeOf(MemberOf(image_program, Uniforms)) == SizeOf(MemberOf(image_program, Uniforms.All)),
             TexturedProgram_AllUniformsArrayLengthMatchesIndividuallyDefinedUniforms);

struct textured_vertex
{
 v4 PUV;
};

struct opengl
{
 renderer_header Header;
 
 render_frame RenderFrame;
 
 u32 MaxTextureCount;
 GLuint *Textures;
 
 u32 MaxBufferCount;
 GLuint *Buffers;
 
 HDC WindowDC;
 
#define OpenGLFunction(Name) func_##Name *Name
 OpenGLFunction(glGenVertexArrays);
 OpenGLFunction(glBindVertexArray);
 OpenGLFunction(glEnableVertexAttribArray);
 OpenGLFunction(glDisableVertexAttribArray);
 OpenGLFunction(glVertexAttribPointer);
 OpenGLFunction(glGetAttribLocation);
 OpenGLFunction(glActiveTexture);
 OpenGLFunction(glGenBuffers);
 OpenGLFunction(glBindBuffer);
 OpenGLFunction(glBufferData);
 OpenGLFunction(glDrawBuffers);
 OpenGLFunction(glCreateProgram);
 OpenGLFunction(glCreateShader);
 OpenGLFunction(glAttachShader);
 OpenGLFunction(glCompileShader);
 OpenGLFunction(glShaderSource);
 OpenGLFunction(glLinkProgram);
 OpenGLFunction(glGetProgramInfoLog);
 OpenGLFunction(glGetShaderInfoLog);
 OpenGLFunction(glUseProgram);
 OpenGLFunction(glValidateProgram);
 OpenGLFunction(glGetUniformLocation);
 OpenGLFunction(glUniform1i);
 OpenGLFunction(glUniform1f);
 OpenGLFunction(glUniform2fv);
 OpenGLFunction(glUniform3fv);
 OpenGLFunction(glUniform4fv);
 OpenGLFunction(glUniformMatrix3fv);
 OpenGLFunction(glUniformMatrix4fv);
 OpenGLFunction(glGetProgramiv);
 OpenGLFunction(glDeleteProgram);
 OpenGLFunction(glDeleteShader);
 OpenGLFunction(glDrawArrays);
 OpenGLFunction(glGenerateMipmap);
 OpenGLFunction(glDebugMessageCallback);
 OpenGLFunction(glVertexAttribDivisor);
 OpenGLFunction(glDrawArraysInstanced);
#undef OpenGLFunction
 
 struct {
  basic_program Program;
  GLuint VBO;
 } Basic;
 
 struct {
  sample_program Program;
  GLuint VertexBuffer;
 } Sample;
 
 struct {
  perfect_circle_program Program;
  GLuint QuadVBO;
  GLuint CircleVBO;
 } PerfectCircle;
 
 struct {
  line_program Program;
  GLuint VertexBuffer;
 } Line;
 
 struct {
  image_program Program;
  GLuint VertexBuffer;
 } Image;
};

#endif //EDITOR_RENDERER_OPENGL_H
