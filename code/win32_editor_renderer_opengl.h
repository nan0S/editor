#ifndef WIN32_EDITOR_RENDERER_OPENGL_H
#define WIN32_EDITOR_RENDERER_OPENGL_H

typedef char GLchar;
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;

typedef BOOL WINAPI func_wglSwapIntervalEXT(int interval);

typedef void WINAPI func_glGenVertexArrays(GLsizei n, GLuint *arrays);
typedef void WINAPI func_glBindVertexArray(GLuint array);
typedef GLint WINAPI func_glGetAttribLocation(GLuint program, const GLchar *name);
typedef void WINAPI func_glEnableVertexAttribArray(GLuint index);
typedef void WINAPI func_glDisableVertexAttribArray(GLuint index);
typedef void WINAPI func_glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
typedef void WINAPI func_glVertexAttribDivisor(GLuint index, GLuint divisor);

typedef void WINAPI func_glActiveTexture(GLenum texture);
typedef void WINAPI func_glGenerateMipmap(GLenum texture);

typedef void WINAPI func_glGenBuffers(GLsizei n, GLuint *buffers);
typedef void WINAPI func_glBindBuffer(GLenum target, GLuint buffer);
typedef void WINAPI func_glBufferData(GLenum target, GLsizeiptr size, const void *data, GLenum usage);
typedef void WINAPI func_glDrawBuffers(GLsizei n, const GLenum *bufs);
typedef void WINAPI func_glDrawArrays(GLenum mode, GLint first, GLsizei count);
typedef void WINAPI func_glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instancecount);

typedef GLuint WINAPI func_glCreateProgram(void);
typedef GLuint WINAPI func_glCreateShader(GLenum type);
typedef void WINAPI func_glAttachShader(GLuint program, GLuint shader);
typedef void WINAPI func_glCompileShader(GLuint shader);
typedef void WINAPI func_glShaderSource(GLuint shader, GLsizei count, GLchar const *const *string, GLint *length);
typedef void WINAPI func_glLinkProgram(GLuint program);
typedef void WINAPI func_glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void WINAPI func_glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void WINAPI func_glUseProgram(GLuint program);
typedef void WINAPI func_glValidateProgram(GLuint program);
typedef void WINAPI func_glGetProgramiv(GLuint program, GLenum pname, GLint *params);
typedef void WINAPI func_glDeleteProgram (GLuint program);
typedef void WINAPI func_glDeleteShader (GLuint shader);

typedef GLint WINAPI func_glGetUniformLocation(GLuint program, const GLchar *name);
typedef void WINAPI func_glUniform1f(GLint location, GLfloat v0);
typedef void WINAPI func_glUniform2fv(GLint location, GLsizei count, const GLfloat *value);
typedef void WINAPI func_glUniform3fv(GLint location, GLsizei count, const GLfloat *value);
typedef void WINAPI func_glUniform4fv(GLint location, GLsizei count, const GLfloat *value);
typedef void WINAPI func_glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void WINAPI func_glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);

typedef BOOL WINAPI func_wglChoosePixelFormatARB(HDC hdc,
                                                 const int *piAttribIList,
                                                 const FLOAT *pfAttribFList,
                                                 UINT nMaxFormats,
                                                 int *piFormats,
                                                 UINT *nNumFormats);

#define GL_DEBUG_CALLBACK(Name) void WINAPI Name(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
typedef GL_DEBUG_CALLBACK(GLDEBUGPROC);
typedef void WINAPI func_glDebugMessageCallback(GLDEBUGPROC *callback, const void *userParam);

#define GL_NUM_EXTENSIONS                 0x821D

#define GL_MAX_COLOR_ATTACHMENTS          0x8CDF
#define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS 0x8B4D

#define GL_TEXTURE_3D                     0x806F

#define GL_TEXTURE0                       0x84C0
#define GL_TEXTURE1                       0x84C1
#define GL_TEXTURE2                       0x84C2
#define GL_TEXTURE3                       0x84C3
#define GL_TEXTURE4                       0x84C4
#define GL_TEXTURE5                       0x84C5
#define GL_TEXTURE6                       0x84C6
#define GL_TEXTURE7                       0x84C7

#define GL_DEBUG_SEVERITY_HIGH            0x9146
#define GL_DEBUG_SEVERITY_MEDIUM          0x9147
#define GL_DEBUG_SEVERITY_LOW             0x9148
#define GL_DEBUG_TYPE_MARKER              0x8268
#define GL_DEBUG_TYPE_PUSH_GROUP          0x8269
#define GL_DEBUG_TYPE_POP_GROUP           0x826A
#define GL_DEBUG_SEVERITY_NOTIFICATION    0x826B

#define GL_DEBUG_SOURCE_API               0x8246
#define GL_DEBUG_SOURCE_APPLICATION       0x824A
#define GL_DEBUG_SOURCE_OTHER             0x824B
#define GL_DEBUG_SOURCE_SHADER_COMPILER   0x8248
#define GL_DEBUG_SOURCE_THIRD_PARTY       0x8249
#define GL_DEBUG_SOURCE_WINDOW_SYSTEM     0x8247

#define GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR 0x824D
#define GL_DEBUG_TYPE_ERROR               0x824C
#define GL_DEBUG_TYPE_MARKER              0x8268
#define GL_DEBUG_TYPE_OTHER               0x8251
#define GL_DEBUG_TYPE_PERFORMANCE         0x8250
#define GL_DEBUG_TYPE_POP_GROUP           0x826A
#define GL_DEBUG_TYPE_PORTABILITY         0x824F
#define GL_DEBUG_TYPE_PUSH_GROUP          0x8269
#define GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR  0x824E

#define GL_DEBUG_OUTPUT_SYNCHRONOUS       0x8242
#define GL_DEBUG_OUTPUT                   0x92E0

#define GL_ARRAY_BUFFER                   0x8892
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_STREAM_DRAW                    0x88E0
#define GL_STREAM_READ                    0x88E1
#define GL_STREAM_COPY                    0x88E2
#define GL_STATIC_DRAW                    0x88E4
#define GL_STATIC_READ                    0x88E5
#define GL_STATIC_COPY                    0x88E6
#define GL_DYNAMIC_DRAW                   0x88E8
#define GL_DYNAMIC_READ                   0x88E9
#define GL_DYNAMIC_COPY                   0x88EA

#define GL_CLAMP_TO_EDGE                  0x812F
#define GL_TEXTURE_MIN_LOD                0x813A
#define GL_TEXTURE_MAX_LOD                0x813B
#define GL_TEXTURE_BASE_LEVEL             0x813C
#define GL_TEXTURE_MAX_LEVEL              0x813D
#define GL_TEXTURE_WRAP_R                 0x8072

#define GL_FRAMEBUFFER_SRGB               0x8DB9
#define GL_SRGB8_ALPHA8                   0x8C43

#define GL_SHADING_LANGUAGE_VERSION       0x8B8C
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_COMPILE_STATUS                 0x8B81
#define GL_LINK_STATUS                    0x8B82
#define GL_VALIDATE_STATUS                0x8B83

#define GL_TEXTURE_2D_ARRAY               0x8C1A

#define GL_FRAMEBUFFER                    0x8D40
#define GL_READ_FRAMEBUFFER               0x8CA8
#define GL_DRAW_FRAMEBUFFER               0x8CA9
#define GL_COLOR_ATTACHMENT0              0x8CE0
#define GL_COLOR_ATTACHMENT1              0x8CE1
#define GL_COLOR_ATTACHMENT2              0x8CE2
#define GL_COLOR_ATTACHMENT3              0x8CE3
#define GL_COLOR_ATTACHMENT4              0x8CE4
#define GL_COLOR_ATTACHMENT5              0x8CE5
#define GL_COLOR_ATTACHMENT6              0x8CE6
#define GL_COLOR_ATTACHMENT7              0x8CE7
#define GL_COLOR_ATTACHMENT8              0x8CE8
#define GL_COLOR_ATTACHMENT9              0x8CE9
#define GL_COLOR_ATTACHMENT10             0x8CEA
#define GL_COLOR_ATTACHMENT11             0x8CEB
#define GL_COLOR_ATTACHMENT12             0x8CEC
#define GL_COLOR_ATTACHMENT13             0x8CED
#define GL_COLOR_ATTACHMENT14             0x8CEE
#define GL_COLOR_ATTACHMENT15             0x8CEF
#define GL_COLOR_ATTACHMENT16             0x8CF0
#define GL_COLOR_ATTACHMENT17             0x8CF1
#define GL_COLOR_ATTACHMENT18             0x8CF2
#define GL_COLOR_ATTACHMENT19             0x8CF3
#define GL_COLOR_ATTACHMENT20             0x8CF4
#define GL_COLOR_ATTACHMENT21             0x8CF5
#define GL_COLOR_ATTACHMENT22             0x8CF6
#define GL_COLOR_ATTACHMENT23             0x8CF7
#define GL_COLOR_ATTACHMENT24             0x8CF8
#define GL_COLOR_ATTACHMENT25             0x8CF9
#define GL_COLOR_ATTACHMENT26             0x8CFA
#define GL_COLOR_ATTACHMENT27             0x8CFB
#define GL_COLOR_ATTACHMENT28             0x8CFC
#define GL_COLOR_ATTACHMENT29             0x8CFD
#define GL_COLOR_ATTACHMENT30             0x8CFE
#define GL_COLOR_ATTACHMENT31             0x8CFF
#define GL_DEPTH_ATTACHMENT               0x8D00
#define GL_FRAMEBUFFER_COMPLETE           0x8CD5

#define GL_DEPTH_COMPONENT16              0x81A5
#define GL_DEPTH_COMPONENT24              0x81A6
#define GL_DEPTH_COMPONENT32              0x81A7
#define GL_DEPTH_COMPONENT32F             0x8CAC

#define GL_RED_INTEGER                    0x8D94
#define GL_GREEN_INTEGER                  0x8D95
#define GL_BLUE_INTEGER                   0x8D96

#define GL_RGBA32F                        0x8814
#define GL_RGB32F                         0x8815
#define GL_RGBA16F                        0x881A
#define GL_RGB16F                         0x881B
#define GL_R8                             0x8229
#define GL_R16                            0x822A
#define GL_RG8                            0x822B
#define GL_RG16                           0x822C
#define GL_R16F                           0x822D
#define GL_R32F                           0x822E
#define GL_RG16F                          0x822F
#define GL_RG32F                          0x8230
#define GL_R8I                            0x8231
#define GL_R8UI                           0x8232
#define GL_R16I                           0x8233
#define GL_R16UI                          0x8234
#define GL_R32I                           0x8235
#define GL_R32UI                          0x8236
#define GL_RG8I                           0x8237
#define GL_RG8UI                          0x8238
#define GL_RG16I                          0x8239
#define GL_RG16UI                         0x823A
#define GL_RG32I                          0x823B
#define GL_RG32UI                         0x823C
#define GL_R11F_G11F_B10F                 0x8C3A

#define GL_MULTISAMPLE                    0x809D
#define GL_SAMPLE_ALPHA_TO_COVERAGE       0x809E
#define GL_SAMPLE_ALPHA_TO_ONE            0x809F
#define GL_SAMPLE_COVERAGE                0x80A0
#define GL_SAMPLE_BUFFERS                 0x80A8
#define GL_SAMPLES                        0x80A9
#define GL_SAMPLE_COVERAGE_VALUE          0x80AA
#define GL_SAMPLE_COVERAGE_INVERT         0x80AB
#define GL_TEXTURE_2D_MULTISAMPLE         0x9100
#define GL_MAX_SAMPLES                    0x8D57
#define GL_MAX_COLOR_TEXTURE_SAMPLES      0x910E
#define GL_MAX_DEPTH_TEXTURE_SAMPLES      0x910F

#define WGL_DRAW_TO_WINDOW_ARB                  0x2001
#define WGL_ACCELERATION_ARB                    0x2003
#define WGL_SUPPORT_OPENGL_ARB                  0x2010
#define WGL_DOUBLE_BUFFER_ARB                   0x2011
#define WGL_PIXEL_TYPE_ARB                      0x2013

#define WGL_TYPE_RGBA_ARB                       0x202B
#define WGL_FULL_ACCELERATION_ARB               0x2027

#define WGL_COLOR_BITS_ARB                      0x2014
#define WGL_RED_BITS_ARB                        0x2015
#define WGL_GREEN_BITS_ARB                      0x2017
#define WGL_BLUE_BITS_ARB                       0x2019
#define WGL_ALPHA_BITS_ARB                      0x201B
#define WGL_DEPTH_BITS_ARB                      0x2022

#define WGL_DRAW_TO_WINDOW_ARB                  0x2001
#define WGL_ACCELERATION_ARB                    0x2003
#define WGL_SUPPORT_OPENGL_ARB                  0x2010
#define WGL_DOUBLE_BUFFER_ARB                   0x2011
#define WGL_PIXEL_TYPE_ARB                      0x2013
#define WGL_SAMPLES_ARB                         0x2042

#define WGL_TYPE_RGBA_ARB                       0x202B

#define WGL_RED_BITS_ARB                        0x2015
#define WGL_GREEN_BITS_ARB                      0x2017
#define WGL_BLUE_BITS_ARB                       0x2019
#define WGL_ALPHA_BITS_ARB                      0x201B
#define WGL_DEPTH_BITS_ARB                      0x2022

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
 
 GLuint VertP_AttrLoc;
 GLuint VertZ_AttrLoc;
 GLuint VertModel_AttrLoc;
 GLuint VertRadiusProper_AttrLoc;
 GLuint VertColor_AttrLoc;
 GLuint VertOutlineColor_AttrLoc;
 GLuint Projection_UniformLoc;
};
struct perfect_circle_program_vertex
{
 v2 P;
 f32 Z;
 union {
  m3x3 Model;
  struct {
   v3 Model0;
   v3 Model1;
   v3 Model2;
  };
 };
 f32 RadiusProper;
 v4 Color;
 v4 OutlineColor;
};

struct opengl
{
 render_frame RenderFrame;
 
 u32 MaxTextureCount;
 GLuint *Textures;
 
 HWND Window;
 HDC WindowDC;
 
#define Win32OpenGLFunction(Name) func_##Name *Name
 Win32OpenGLFunction(wglSwapIntervalEXT);
 Win32OpenGLFunction(glGenVertexArrays);
 Win32OpenGLFunction(glBindVertexArray);
 Win32OpenGLFunction(glEnableVertexAttribArray);
 Win32OpenGLFunction(glDisableVertexAttribArray);
 Win32OpenGLFunction(glVertexAttribPointer);
 Win32OpenGLFunction(glGetAttribLocation);
 Win32OpenGLFunction(glActiveTexture);
 Win32OpenGLFunction(glGenBuffers);
 Win32OpenGLFunction(glBindBuffer);
 Win32OpenGLFunction(glBufferData);
 Win32OpenGLFunction(glDrawBuffers);
 Win32OpenGLFunction(glCreateProgram);
 Win32OpenGLFunction(glCreateShader);
 Win32OpenGLFunction(glAttachShader);
 Win32OpenGLFunction(glCompileShader);
 Win32OpenGLFunction(glShaderSource);
 Win32OpenGLFunction(glLinkProgram);
 Win32OpenGLFunction(glGetProgramInfoLog);
 Win32OpenGLFunction(glGetShaderInfoLog);
 Win32OpenGLFunction(glUseProgram);
 Win32OpenGLFunction(glValidateProgram);
 Win32OpenGLFunction(glGetUniformLocation);
 Win32OpenGLFunction(glUniform1f);
 Win32OpenGLFunction(glUniform2fv);
 Win32OpenGLFunction(glUniform3fv);
 Win32OpenGLFunction(glUniform4fv);
 Win32OpenGLFunction(glUniformMatrix3fv);
 Win32OpenGLFunction(glUniformMatrix4fv);
 Win32OpenGLFunction(glGetProgramiv);
 Win32OpenGLFunction(glDeleteProgram);
 Win32OpenGLFunction(glDeleteShader);
 Win32OpenGLFunction(glDrawArrays);
 Win32OpenGLFunction(wglChoosePixelFormatARB);
 Win32OpenGLFunction(glGenerateMipmap);
 Win32OpenGLFunction(glDebugMessageCallback);
 Win32OpenGLFunction(glVertexAttribDivisor);
 Win32OpenGLFunction(glDrawArraysInstanced);
#undef Win32OpenGLFunction
 
 struct {
  basic_program Program;
  GLuint VAO;
  GLuint VBO;
 } Basic;
 
 struct {
  perfect_circle_program Program;
  GLuint VAO;
  GLuint QuadVBO;
  GLuint CircleVBO;
 } PerfectCircle;
};

#endif //WIN32_EDITOR_RENDERER_OPENGL_H
