#ifndef WIN32_EDITOR_RENDERER_OPENGL_H
#define WIN32_EDITOR_RENDERER_OPENGL_H

typedef char GLchar;
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;

typedef BOOL WINAPI func_wglSwapIntervalEXT(int interval);
typedef BOOL WINAPI func_wglChoosePixelFormatARB(HDC hdc,
                                                 const int *piAttribIList,
                                                 const FLOAT *pfAttribFList,
                                                 UINT nMaxFormats,
                                                 int *piFormats,
                                                 UINT *nNumFormats);

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

#define GL_DEBUG_CALLBACK(Name) void WINAPI Name(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
typedef GL_DEBUG_CALLBACK(GLDEBUGPROC);
typedef void WINAPI func_glDebugMessageCallback(GLDEBUGPROC *callback, const void *userParam);

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

struct win32_opengl_renderer
{
 HDC WindowDC;
 func_wglChoosePixelFormatARB *wglChoosePixelFormatARB;
 func_wglSwapIntervalEXT *wglSwapIntervalEXT;
};

#endif //WIN32_EDITOR_RENDERER_OPENGL_H