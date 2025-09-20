/* ========================================================================
   Parametric Curve Editor
   Master's Thesis by Hubert Obrzut
   Supervisor: Paweł Woźny
   University of Wrocław
   Faculty of Mathematics and Computer Science
   Institute of Computer Science
   Date: September 2025
   ======================================================================== */

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
typedef HGLRC WINAPI func_wglCreateContextAttribsARB(HDC hDC, HGLRC hShareContext,
                                                     const int *attribList);

typedef void func_glShaderSource(GLuint shader, GLsizei count, GLchar const *const *string, GLint *length);

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

#define WGL_CONTEXT_MAJOR_VERSION_ARB             0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB             0x2092
#define WGL_CONTEXT_FLAGS_ARB                     0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB              0x9126
#define WGL_CONTEXT_DEBUG_BIT_ARB                 0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB    0x0002
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB          0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002

struct win32_opengl_renderer
{
 HDC WindowDC;
 func_wglChoosePixelFormatARB *wglChoosePixelFormatARB;
 func_wglSwapIntervalEXT *wglSwapIntervalEXT;
 func_wglCreateContextAttribsARB *wglCreateContextAttribsARB;
};

#endif //WIN32_EDITOR_RENDERER_OPENGL_H