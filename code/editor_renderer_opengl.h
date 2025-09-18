#ifndef EDITOR_RENDERER_OPENGL_H
#define EDITOR_RENDERER_OPENGL_H

typedef void func_glGenVertexArrays(GLsizei n, GLuint *arrays);
typedef void func_glBindVertexArray(GLuint array);
typedef GLint func_glGetAttribLocation(GLuint program, const GLchar *name);
typedef void func_glEnableVertexAttribArray(GLuint index);
typedef void func_glDisableVertexAttribArray(GLuint index);
typedef void func_glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
typedef void func_glVertexAttribDivisor(GLuint index, GLuint divisor);

typedef void func_glGenBuffers(GLsizei n, GLuint *buffers);
typedef void func_glBindBuffer(GLenum target, GLuint buffer);
typedef void func_glBufferData(GLenum target, GLsizeiptr size, const void *data, GLenum usage);
typedef void func_glDrawBuffers(GLsizei n, const GLenum *bufs);
typedef void func_glDrawArrays(GLenum mode, GLint first, GLsizei count);
typedef void func_glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instancecount);
typedef void func_glDrawArraysInstancedBaseInstance(GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance);

typedef void func_glActiveTexture(GLenum texture);
typedef void func_glGenerateMipmap(GLenum texture);

typedef GLuint func_glCreateProgram(void);
typedef GLuint func_glCreateShader(GLenum type);
typedef void func_glAttachShader(GLuint program, GLuint shader);
typedef void func_glCompileShader(GLuint shader);
typedef void func_glShaderSource(GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
typedef void func_glLinkProgram(GLuint program);
typedef void func_glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void func_glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void func_glUseProgram(GLuint program);
typedef void func_glValidateProgram(GLuint program);
typedef void func_glGetProgramiv(GLuint program, GLenum pname, GLint *params);
typedef void func_glDeleteProgram (GLuint program);
typedef void func_glDeleteShader (GLuint shader);
typedef GLint func_glGetUniformLocation(GLuint program, const GLchar *name);
typedef void func_glUniform1i(GLint location, GLint v0);
typedef void func_glUniform1f(GLint location, GLfloat v0);
typedef void func_glUniform2fv(GLint location, GLsizei count, const GLfloat *value);
typedef void func_glUniform3fv(GLint location, GLsizei count, const GLfloat *value);
typedef void func_glUniform4fv(GLint location, GLsizei count, const GLfloat *value);
typedef void func_glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void func_glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);

#define GL_DEBUG_CALLBACK(Name) void Name(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
typedef GL_DEBUG_CALLBACK(gl_debug_callback);
typedef void func_glDebugMessageCallback(gl_debug_callback *callback, const void *userParam);

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
StaticAssert(SizeOf(MemberOf(perfect_circle_program, Attributes)) ==
             SizeOf(MemberOf(perfect_circle_program, Attributes.All)),
             PerfectCircleProgram_AllAttributesArrayLengthMatchesIndividuallyDefinedAttributes);
StaticAssert(SizeOf(MemberOf(perfect_circle_program, Uniforms)) ==
             SizeOf(MemberOf(perfect_circle_program, Uniforms.All)),
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
StaticAssert(SizeOf(MemberOf(line_program, Attributes)) ==
             SizeOf(MemberOf(line_program, Attributes.All)),
             LineProgram_AllAttributesArrayLengthMatchesIndividuallyDefinedAttributes);
StaticAssert(SizeOf(MemberOf(line_program, Uniforms)) ==
             SizeOf(MemberOf(line_program, Uniforms.All)),
             LineProgram_AllUniformsArrayLengthMatchesIndividuallyDefinedUniforms);

struct image_vertex
{
 v2 P;
 v2 UV;
};
struct image_program
{
 GLuint ProgramHandle;
 
 union {
  struct {
   GLuint VertP_AttrLoc;
   GLuint VertUV_AttrLoc;
   GLuint VertZ_AttrLoc;
   GLuint VertModel0_AttrLoc;
   GLuint VertModel1_AttrLoc;
   GLuint VertModel2_AttrLoc;
  };
  GLuint All[6];
 } Attributes;
 
 union {
  struct {
   GLuint Projection_UniformLoc;
   GLuint Samplers[8];
  };
  GLuint All[9];
 } Uniforms;
};
StaticAssert(SizeOf(MemberOf(image_program, Attributes)) ==
             SizeOf(MemberOf(image_program, Attributes.All)),
             ImageProgram_AllAttributesArrayLengthMatchesIndividuallyDefinedAttributes);
StaticAssert(SizeOf(MemberOf(image_program, Uniforms)) ==
             SizeOf(MemberOf(image_program, Uniforms.All)),
             ImageProgram_AllUniformsArrayLengthMatchesIndividuallyDefinedUniforms);

struct vertex_program
{
 GLuint ProgramHandle;
 
 union {
  struct {
   GLuint VertP_AttrLoc;
   GLuint VertZ_AttrLoc;
   GLuint VertColor_AttrLoc;
  };
  GLuint All[3];
 } Attributes;
 
 union {
  struct {
   GLuint Projection_UniformLoc;
  };
  GLuint All[1];
 } Uniforms;
};
StaticAssert(SizeOf(MemberOf(vertex_program, Attributes)) ==
             SizeOf(MemberOf(vertex_program, Attributes.All)),
             VertexProgram_AllAttributesArrayLengthMatchesIndividuallyDefinedAttributes);
StaticAssert(SizeOf(MemberOf(vertex_program, Uniforms)) ==
             SizeOf(MemberOf(vertex_program, Uniforms.All)),
             VertexProgram_AllUniformsArrayLengthMatchesIndividuallyDefinedUniforms);

struct opengl
{
 renderer_header Header;
 
 render_frame RenderFrame;
 
 u32 MaxTextureCount;
 GLuint *Textures;
 
 u32 MaxBufferCount;
 GLuint *Buffers;
 
 u32 MaxTextureSlots;
 
 b32 PolygonModeIsWireFrame;
 
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
 OpenGLFunction(glDrawArraysInstancedBaseInstance);
#undef OpenGLFunction
 
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
  GLuint ImageBuffer;
 } Image;
 
 struct {
  vertex_program Program;
  GLuint VertexBuffer;
 } Vertex;
};

#endif //EDITOR_RENDERER_OPENGL_H
