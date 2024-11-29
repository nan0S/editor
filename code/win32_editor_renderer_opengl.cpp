#include <windows.h>
#include <gl/gl.h>

#include "base_ctx_crack.h"
#include "base_core.h"
#include "base_string.h"
#include "base_os.h"

#include "editor_memory.h"
#include "editor_math.h"
#include "editor_imgui_bindings.h"
#include "editor_platform.h"
#include "editor_renderer.h"

#include "win32_editor_renderer.h"
#include "win32_editor_renderer_opengl.h"
#include "win32_editor_imgui_bindings.h"
#include "win32_shared.h"

#include "base_core.cpp"
#include "base_string.cpp"
#include "base_os.cpp"

#include "editor_memory.cpp"
#include "editor_math.cpp"

#include "editor_third_party_inc.h"

platform_api Platform;

GL_DEBUG_CALLBACK(OpenGLDebugCallback)
{
 // ignore non-significant error/warning codes
 if(id == 131169 || id == 131185 || id == 131218 || id == 131204) return; 
 
 char const *SourceStr = 0;
 switch (source)
 {
  case GL_DEBUG_SOURCE_API:             SourceStr = "Source: API"; break;
  case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   SourceStr = "Source: Window System"; break;
  case GL_DEBUG_SOURCE_SHADER_COMPILER: SourceStr = "Source: Shader Compiler"; break;
  case GL_DEBUG_SOURCE_THIRD_PARTY:     SourceStr = "Source: Third Party"; break;
  case GL_DEBUG_SOURCE_APPLICATION:     SourceStr = "Source: Application"; break;
  case GL_DEBUG_SOURCE_OTHER:           SourceStr = "Source: Other"; break;
 }
 
 char const *TypeStr = 0;
 switch (type)
 {
  case GL_DEBUG_TYPE_ERROR:               TypeStr = "Type: Error"; break;
  case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: TypeStr = "Type: Deprecated Behaviour"; break;
  case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  TypeStr = "Type: Undefined Behaviour"; break; 
  case GL_DEBUG_TYPE_PORTABILITY:         TypeStr = "Type: Portability"; break;
  case GL_DEBUG_TYPE_PERFORMANCE:         TypeStr = "Type: Performance"; break;
  case GL_DEBUG_TYPE_MARKER:              TypeStr = "Type: Marker"; break;
  case GL_DEBUG_TYPE_PUSH_GROUP:          TypeStr = "Type: Push Group"; break;
  case GL_DEBUG_TYPE_POP_GROUP:           TypeStr = "Type: Pop Group"; break;
  case GL_DEBUG_TYPE_OTHER:               TypeStr = "Type: Other"; break;
 }
 
 char const *SeverityStr = 0;
 switch (severity)
 {
  case GL_DEBUG_SEVERITY_HIGH:         SeverityStr = "Severity: high"; break;
  case GL_DEBUG_SEVERITY_MEDIUM:       SeverityStr = "Severity: medium"; break;
  case GL_DEBUG_SEVERITY_LOW:          SeverityStr = "Severity: low"; break;
  case GL_DEBUG_SEVERITY_NOTIFICATION: SeverityStr = "Severity: notification"; break;
 }
 
 Trap;
 
 OS_PrintDebugF("%s %s %s\n", SourceStr, TypeStr, SeverityStr);
}

#define GL_INVALID_FRAMEBUFFER_OPERATION 0x0506
internal void
OpenGLCheckErrors(void)
{
 GLenum Error = glGetError();
 if (Error != GL_NO_ERROR)
 {
  char const *error = 0;
  switch (Error)
  {
   case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
   case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
   case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
   case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
   case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
   case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
   case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
  }
  Trap;
 }
}

#define GL_CALL(Expr) Expr; OpenGLCheckErrors()

internal void
Win32SetPixelFormat(opengl *OpenGL, HDC WindowDC)
{
 WIN32_BEGIN_DEBUG_BLOCK(SetPixelFormat);
 
 int SuggestedPixelFormatIndex = 0;
 UINT ExtendedPicked = 0;
 
 if (OpenGL->wglChoosePixelFormatARB)
 {
  int AttribIList[] =
  {
   WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
   WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
   WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
   WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
   WGL_COLOR_BITS_ARB, 32,
   WGL_ALPHA_BITS_ARB, 8,
   WGL_DEPTH_BITS_ARB, 24,
   // NOTE(hbr): When I set WGL_SAMPLES_ARB to some values (e.g. 16), ImGui text becomes blurry.
   // I don't know why that is.
   WGL_SAMPLES_ARB, 8,
   // NOTE(hbr): I don't know if this is really needed, but Casey uses it
   WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
   0
  };
  
  OpenGL->wglChoosePixelFormatARB(WindowDC,
                                  AttribIList,
                                  0,
                                  1,
                                  &SuggestedPixelFormatIndex,
                                  &ExtendedPicked);
 }
 
 if (!ExtendedPicked)
 {
  PIXELFORMATDESCRIPTOR DesiredPixelFormat = {};
  DesiredPixelFormat.nSize = SizeOf(DesiredPixelFormat);
  DesiredPixelFormat.nVersion = 1;
  DesiredPixelFormat.iPixelType = PFD_TYPE_RGBA;
  DesiredPixelFormat.dwFlags = (PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER);
  DesiredPixelFormat.cColorBits = 32;
  DesiredPixelFormat.cAlphaBits = 8;
  DesiredPixelFormat.cDepthBits = 24;
  DesiredPixelFormat.iLayerType = PFD_MAIN_PLANE;
  
  SuggestedPixelFormatIndex = ChoosePixelFormat(WindowDC, &DesiredPixelFormat);
 }
 
 PIXELFORMATDESCRIPTOR SuggestedPixelFormat;
 DescribePixelFormat(WindowDC, SuggestedPixelFormatIndex, SizeOf(SuggestedPixelFormat), &SuggestedPixelFormat);
 SetPixelFormat(WindowDC, SuggestedPixelFormatIndex, &SuggestedPixelFormat);
 
 WIN32_END_DEBUG_BLOCK(SetPixelFormat);
}

internal GLuint
Win32OpenGLCreateProgram(opengl *OpenGL, char const *VertexCode, char const *FragmentCode)
{
 char const *ShaderCodeHeader = R"FOO(
 #version 330
 
#define s32 int
#define u32 uint
  #define f32 float
  #define v2 vec2
  #define v3 vec3
  #define v4 vec4
  #define v2s ivec2
  #define v3s ivec3
  #define v4s ivec4
  #define m3x3 mat3
  #define m4x4 mat4
  #define Square(X) ((X)*(X))
  #define Dot(V) dot(V)
  #define Length(V) length(V)
  #define Length2(V) dot(V)
  )FOO";
 
 GLuint VertexShaderID = OpenGL->glCreateShader(GL_VERTEX_SHADER);
 GLchar *VertexShaderCode[] =
 {
  Cast(char *)ShaderCodeHeader,
  Cast(char *)VertexCode,
 };
 OpenGL->glShaderSource(VertexShaderID, ArrayCount(VertexShaderCode), VertexShaderCode, 0);
 OpenGL->glCompileShader(VertexShaderID);
 
 GLuint FragmentShaderID = OpenGL->glCreateShader(GL_FRAGMENT_SHADER);
 GLchar *FragmentShaderCode[] =
 {
  Cast(char *)ShaderCodeHeader,
  Cast(char *)FragmentCode,
 };
 OpenGL->glShaderSource(FragmentShaderID, ArrayCount(FragmentShaderCode), FragmentShaderCode, 0);
 OpenGL->glCompileShader(FragmentShaderID);
 
 GLuint ProgramID = OpenGL->glCreateProgram();
 OpenGL->glAttachShader(ProgramID, VertexShaderID);
 OpenGL->glAttachShader(ProgramID, FragmentShaderID);
 OpenGL->glLinkProgram(ProgramID);
 
 OpenGL->glValidateProgram(ProgramID);
 GLint Linked = false;
 OpenGL->glGetProgramiv(ProgramID, GL_LINK_STATUS, &Linked);
 if(!Linked)
 {
  GLsizei Ignored;
  char VertexErrors[4096];
  char FragmentErrors[4096];
  char ProgramErrors[4096];
  OpenGL->glGetShaderInfoLog(VertexShaderID, sizeof(VertexErrors), &Ignored, VertexErrors);
  OpenGL->glGetShaderInfoLog(FragmentShaderID, sizeof(FragmentErrors), &Ignored, FragmentErrors);
  OpenGL->glGetProgramInfoLog(ProgramID, sizeof(ProgramErrors), &Ignored, ProgramErrors);
  
  Assert(!"Shader validation failed");
 }
 
 OpenGL->glDeleteShader(VertexShaderID);
 OpenGL->glDeleteShader(FragmentShaderID);
 
 return ProgramID;
}

internal basic_program
CompileBasicProgram(opengl *OpenGL)
{
 basic_program Result = {};
 
 char const *VertexShader = R"FOO(
in v2 VertP;
in v4 VertColor;

out v4 FragColor;

void main(void) {
gl_Position = v4(VertP, 0, 1);
FragColor = VertColor;
}
)FOO";
 
 char const *FragmentShader = R"FOO(
  in v4 FragColor;

out v4 OutColor;
 
 void main(void) {
OutColor = FragColor;
 }
 )FOO";
 
 GLuint ProgramHandle = Win32OpenGLCreateProgram(OpenGL, VertexShader, FragmentShader);
 Result.ProgramHandle = ProgramHandle;
 Result.VertP_AttrLoc = OpenGL->glGetAttribLocation(ProgramHandle, "VertP");
 Result.VertColor_AttrLoc = OpenGL->glGetAttribLocation(ProgramHandle, "VertColor");
 
 return Result;
}

internal sample_program
CompileSampleProgram(opengl *OpenGL)
{
 sample_program Result = {};
 
 char const *VertexShader = R"FOO(
in v2 VertP;
in v2 VertUV;

out v2 FragUV;

uniform mat4 Transform;

void main(void) {
gl_Position = Transform * v4(VertP, 0, 1);
FragUV = VertUV;
}

)FOO";
 
 char const *FragmentShader = R"FOO(
in v2 FragUV;
out v4 FragColor;

uniform v4 Color;
uniform sampler2D Sampler;

void main(void) {
//FragColor = texture(Sampler, FragUV);
FragColor = Color;
}

)FOO";
 
 GLuint ProgramHandle = Win32OpenGLCreateProgram(OpenGL, VertexShader, FragmentShader);
 Result.ProgramHandle = ProgramHandle;
 Result.VertP_AttrLoc = OpenGL->glGetAttribLocation(ProgramHandle, "VertP");
 Result.VertUV_AttrLoc = OpenGL->glGetAttribLocation(ProgramHandle, "VertUV");
 Result.Transform_UniformLoc = OpenGL->glGetUniformLocation(ProgramHandle, "Transform");
 Result.Color_UniformLoc = OpenGL->glGetUniformLocation(ProgramHandle, "Color");
 
 return Result;
}

internal perfect_circle_program
CompilePerfectCircleProgram(opengl *OpenGL)
{
 perfect_circle_program Result = {};
 
 char const *VertexShader = R"FOO(
in v2 VertP;
in f32 VertZ;
in m3x3 VertModel;
in f32 VertRadiusProper;
in v4 VertColor;
in v4 VertOutlineColor;

out v2 FragP;
flat out f32 FragRadiusProper;
flat out v4 FragColor;
flat out v4 FragOutlineColor;

uniform m3x3 Projection;

void main(void)
 {
m3x3 Transform = Projection * VertModel;
v3 P = Transform * v3(VertP, 1);
 gl_Position = v4(P.xy, VertZ, P.z);

 FragP = VertP;
 FragRadiusProper = VertRadiusProper;
FragColor = VertColor;
FragOutlineColor = VertOutlineColor;
}
)FOO";
 
 char const *FragmentShader = R"FOO(
in v2 FragP;
flat in f32 FragRadiusProper;
flat in v4 FragColor;
flat in v4 FragOutlineColor;
 
out v4 OutColor;

 void main(void)
{
f32 Dist2 = dot(FragP, FragP);
bool InsideProperCircle = (Dist2 <= Square(FragRadiusProper));
bool InsideOutline = (Dist2 <= Square(1.0f));

if (InsideProperCircle) {
OutColor = FragColor;
}
else if (InsideOutline) {
OutColor = FragOutlineColor;
}
else {
  OutColor = v4(0, 0, 0, 0);
  }
 }
 )FOO";
 
 GLuint ProgramHandle = Win32OpenGLCreateProgram(OpenGL, VertexShader, FragmentShader);
 Result.ProgramHandle = ProgramHandle;
 
 char const *AttrNames[] =
 {
  "VertP",
  "VertZ",
  "VertModel",
  "VertModel",
  "VertModel",
  "VertRadiusProper",
  "VertColor",
  "VertOutlineColor",
 };
 StaticAssert(ArrayCount(AttrNames) == ArrayCount(MemberOf(perfect_circle_program, Attributes.All)),
              AttrNamesLengthMatchesDefinedAttributes);
 
 for (u32 AttrIndex = 0;
      AttrIndex < ArrayCount(AttrNames);
      ++AttrIndex)
 {
  char const *AttrName = AttrNames[AttrIndex];
  GLuint Attr = OpenGL->glGetAttribLocation(ProgramHandle, AttrName);
  Result.Attributes.All[AttrIndex] = Attr;
  Assert(Attr != -1);
 }
 Result.Attributes.VertModel1_AttrLoc = Result.Attributes.VertModel0_AttrLoc + 1;
 Result.Attributes.VertModel2_AttrLoc = Result.Attributes.VertModel0_AttrLoc + 2;
 
 Result.Projection_UniformLoc = OpenGL->glGetUniformLocation(ProgramHandle, "Projection");
 Assert(Result.Projection_UniformLoc != -1);
 
 return Result;
}

#define Win32OpenGLFunction(Name) OpenGL->Name = Cast(func_##Name *)wglGetProcAddress(#Name);
DLL_EXPORT
WIN32_RENDERER_INIT(Win32RendererInit)
{
 Platform = Memory->PlatformAPI;
 
 opengl *OpenGL = PushStruct(Arena, opengl);
 OpenGL->Window = Window;
 OpenGL->WindowDC = WindowDC;
 
 //- create false context to retrieve extensions pointers
 {
  WNDCLASSA WindowClass = {};
  { 
   WindowClass.lpfnWndProc = DefWindowProcA;
   WindowClass.hInstance = GetModuleHandle(0);
   WindowClass.lpszClassName = "Win32EditorWGLLoader";
  }
  
  if (RegisterClassA(&WindowClass))
  {
   HWND FalseWindow =
    CreateWindowExA(0,
                    WindowClass.lpszClassName,
                    "Parametric Curves Editor",
                    0,
                    CW_USEDEFAULT,
                    CW_USEDEFAULT,
                    CW_USEDEFAULT,
                    CW_USEDEFAULT,
                    0,
                    0,
                    WindowClass.hInstance,
                    0);
   
   HDC FalseWindowDC = GetWindowDC(FalseWindow);
   Win32SetPixelFormat(OpenGL, FalseWindowDC);
   
   HGLRC FalseOpenGLRC = wglCreateContext(FalseWindowDC);
   if (wglMakeCurrent(FalseWindowDC, FalseOpenGLRC))
   {
    Win32OpenGLFunction(wglChoosePixelFormatARB);
   }
   
   wglDeleteContext(FalseOpenGLRC);
   ReleaseDC(FalseWindow, FalseWindowDC);
   DestroyWindow(FalseWindow);
  }
 }
 
 //- initialize opengl context 
 Win32SetPixelFormat(OpenGL, WindowDC);
 HGLRC OpenGLRC = wglCreateContext(WindowDC);
 if (wglMakeCurrent(WindowDC, OpenGLRC))
 {
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
  glEnable(GL_MULTISAMPLE);
  // NOTE(hbr): So that glEnable,glEnd with glBindTexture works. When using shaders it is not needed.
  glEnable(GL_TEXTURE_2D);
  
  Win32OpenGLFunction(wglSwapIntervalEXT);
  Win32OpenGLFunction(glGenVertexArrays);
  Win32OpenGLFunction(glBindVertexArray);
  Win32OpenGLFunction(glEnableVertexAttribArray);
  Win32OpenGLFunction(glDisableVertexAttribArray);
  Win32OpenGLFunction(glVertexAttribPointer);
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
  Win32OpenGLFunction(glGetUniformLocation);
  Win32OpenGLFunction(glUniform1f);
  Win32OpenGLFunction(glUniform2fv);
  Win32OpenGLFunction(glUniform3fv);
  Win32OpenGLFunction(glUniform4fv);
  Win32OpenGLFunction(glUniformMatrix3fv);
  Win32OpenGLFunction(glUniformMatrix4fv);
  Win32OpenGLFunction(glGetAttribLocation);
  Win32OpenGLFunction(glValidateProgram);
  Win32OpenGLFunction(glGetProgramiv);
  Win32OpenGLFunction(glDeleteProgram);
  Win32OpenGLFunction(glDeleteShader);
  Win32OpenGLFunction(glDrawArrays);
  Win32OpenGLFunction(glGenerateMipmap);
  Win32OpenGLFunction(glDebugMessageCallback);
  Win32OpenGLFunction(glVertexAttribDivisor);
  Win32OpenGLFunction(glDrawArraysInstanced);
  
  if (OpenGL->wglSwapIntervalEXT)
  {
   // NOTE(hbr): disable VSync
   OpenGL->wglSwapIntervalEXT(0);
  }
  
  if (OpenGL->glDebugMessageCallback)
  {
   glEnable(GL_DEBUG_OUTPUT);
   glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
   OpenGL->glDebugMessageCallback(OpenGLDebugCallback, 0);
  }
  
  GLuint DummyVAO;
  OpenGL->glGenVertexArrays(1, &DummyVAO);
  OpenGL->glBindVertexArray(DummyVAO);
  
  //- allocate texutre indices
  u32 TextureCount = Memory->Limits.MaxTextureCount;
  GLuint *Textures = PushArray(Arena, TextureCount, GLuint);
  OpenGL->MaxTextureCount = TextureCount;
  OpenGL->Textures = Textures;
  
  glGenTextures(Cast(GLsizei)TextureCount, Textures);
  for (u32 TextureIndex = 0;
       TextureIndex < TextureCount;
       ++TextureIndex)
  {
   glBindTexture(GL_TEXTURE_2D, Textures[TextureIndex]);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  }
  
  //- compile and initialize perfect circle program
  {
   OpenGL->PerfectCircle.Program = CompilePerfectCircleProgram(OpenGL);
   
   OpenGL->glGenBuffers(1, &OpenGL->PerfectCircle.QuadVBO);
   OpenGL->glGenBuffers(1, &OpenGL->PerfectCircle.CircleVBO);
   
   v2 Vertices[] =
   {
    V2(-1, -1),
    V2( 1, -1),
    V2( 1,  1),
    V2(-1,  1),
   };
   OpenGL->glBindBuffer(GL_ARRAY_BUFFER, OpenGL->PerfectCircle.QuadVBO);
   OpenGL->glBufferData(GL_ARRAY_BUFFER, SizeOf(Vertices), Vertices, GL_STATIC_DRAW);
  }
 }
 
 return Cast(renderer *)OpenGL;
}
#undef Win32OpenGLFunction

DLL_EXPORT
RENDERER_BEGIN_FRAME(Win32RendererBeginFrame)
{
 Platform = Memory->PlatformAPI;
 
 opengl *OpenGL = Cast(opengl *)Renderer;
 
 render_frame *RenderFrame = &OpenGL->RenderFrame;
 RenderFrame->CommandCount = 0;
 RenderFrame->Commands = Memory->CommandBuffer;
 RenderFrame->MaxCommandCount = Memory->MaxCommandCount;
 RenderFrame->CircleCount = 0;
 RenderFrame->Circles = Memory->CircleBuffer;
 RenderFrame->MaxCircleCount = Memory->MaxCircleCount;
 RenderFrame->WindowDim = WindowDim;
 
 Memory->ImGuiBindings.NewFrame();
 
 return RenderFrame;
}

internal int RenderCommandCmp(render_command *A, render_command *B) { return Cmp(A->ZOffset, B->ZOffset); }

internal m4x4
M3x3ToM4x4OpenGL(m3x3 M)
{
 // NOTE(hbr): OpenGL matrices are column-major
 M = Transpose3x3(M);
 m4x4 R = {
  { {M.M[0][0], M.M[0][1], 0, M.M[0][2]},
   {M.M[1][0], M.M[1][1], 0, M.M[1][2]},
   {        0,         0, 1,         0},
   {M.M[2][0], M.M[2][1], 0, M.M[2][2]}}
 };
 
 return R;
}

DLL_EXPORT
RENDERER_END_FRAME(Win32RendererEndFrame)
{
 opengl *OpenGL = Cast(opengl *)Renderer;
 texture_transfer_queue *Queue = &Memory->TextureQueue;
 GLuint *Textures = OpenGL->Textures;
 m4x4 Projection = M3x3ToM4x4OpenGL(Frame->Proj);
 m3x3 Proj3x3 = Frame->Proj;
 
 v4 Clear = Frame->ClearColor;
 glClearColor(Clear.R, Clear.G, Clear.B, Clear.A);
 glClear(GL_COLOR_BUFFER_BIT);
 glViewport(0, 0, Frame->WindowDim.X, Frame->WindowDim.Y);
 
 //- uploads textures into GPU
 for (u32 OpIndex = 0;
      OpIndex < Queue->OpCount;
      ++OpIndex)
 {
  texture_transfer_op *Op = Queue->Ops + OpIndex;
  Assert(Op->TextureIndex < OpenGL->MaxTextureCount);
  
  glBindTexture(GL_TEXTURE_2D, Textures[Op->TextureIndex]);
  glTexImage2D(GL_TEXTURE_2D,
               0,
               GL_RGBA,
               Cast(u32)Op->Width,
               Cast(u32)Op->Height,
               0,
               GL_RGBA,
               GL_UNSIGNED_BYTE,
               Op->Pixels);
  OpenGL->glGenerateMipmap(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, 0);
 }
 Queue->OpCount = 0;
 Queue->TransferMemoryUsed = 0;
 
 //- draw
 QuickSort(Frame->Commands, Frame->CommandCount, render_command, RenderCommandCmp);
 for (u32 CommandIndex = 0;
      CommandIndex < Frame->CommandCount;
      ++CommandIndex)
 {
  render_command *Command = Frame->Commands + CommandIndex;
  
  glMatrixMode(GL_PROJECTION);
  glLoadMatrixf(Cast(f32 *)Projection.M);
  
  m4x4 Model = M3x3ToM4x4OpenGL(Command->ModelXForm);
  glMatrixMode(GL_MODELVIEW);
  glLoadMatrixf(Cast(f32 *)Model.M);
  
  switch (Command->Type)
  {
   case RenderCommand_VertexArray: {
    render_command_vertex_array *Array = &Command->VertexArray;
    
    int glPrimitive = 0;
    switch (Array->Primitive)
    {
     case Primitive_TriangleStrip: {glPrimitive = GL_TRIANGLE_STRIP;}break;
     case Primitive_Triangles:     {glPrimitive = GL_TRIANGLES;}break;
    }
    
    v4 Color = V4(1, 0, 1, 1);
    m4x4 Transform = Projection;
    
    local b32 Init = false;
    local GLuint VAO = 0;
    local GLuint VBO = 0;
    local sample_program Program = {};
    
    if (!Init)
    {
     Program = CompileSampleProgram(OpenGL);
     OpenGL->glGenBuffers(1, &VBO);
     Init = true;
    }
    
    OpenGL->glBindBuffer(GL_ARRAY_BUFFER, VBO);
    OpenGL->glBufferData(GL_ARRAY_BUFFER,
                         Array->VertexCount * SizeOf(Array->Vertices[0]),
                         Array->Vertices, GL_DYNAMIC_DRAW);
    
    OpenGL->glEnableVertexAttribArray(Program.VertP_AttrLoc);
    OpenGL->glVertexAttribPointer(Program.VertP_AttrLoc, 2, GL_FLOAT, GL_FALSE, SizeOf(v2), 0);
    //OpenGL->glEnableVertexAttribArray(Program.VertUV_AttrLoc);
    //OpenGL->glVertexAttribPointer(Program.VertUV_AttrLoc, 2, GL_FLOAT, GL_FALSE, 2 * SizeOf(v2), Cast(void *)SizeOf(v2));
    
    OpenGL->glUseProgram(Program.ProgramHandle);
    OpenGL->glUniformMatrix4fv(Program.Transform_UniformLoc, 1, GL_FALSE, Cast(f32 *)Transform.M);
    OpenGL->glUniform4fv(Program.Color_UniformLoc, 1, Cast(f32 *)Color.E);
    
    OpenGL->glDrawArrays(glPrimitive, 0, Array->VertexCount);
    
    OpenGL->glUseProgram(0);
    OpenGL->glBindBuffer(GL_ARRAY_BUFFER, 0);
    
   }break;
   
   case RenderCommand_Rectangle: {
    render_command_rectangle *Rect = &Command->Rectangle;
    
    glBegin(GL_QUADS);
    glColor4fv(Rect->Color.E);
    glVertex2f(-1, -1);
    glVertex2f(1, -1);
    glVertex2f(1, 1);
    glVertex2f(-1, 1);
    glEnd();
   }break;
   
   case RenderCommand_Triangle: {
    render_command_triangle *Tri = &Command->Triangle;
    
    glBegin(GL_TRIANGLES);
    glColor4fv(Tri->Color.E);
    glVertex2fv(Tri->P0.E);
    glVertex2fv(Tri->P1.E);
    glVertex2fv(Tri->P2.E);
    glEnd();
   }break;
   
   case RenderCommand_Image: {
    render_command_image *Image = &Command->Image;
    
    glBindTexture(GL_TEXTURE_2D, Textures[Image->TextureIndex]);
    
    glBegin(GL_QUADS);
    glColor4f(1, 1, 1, 1);
    
    glTexCoord2f(0, 0);
    glVertex2f(-1, -1);
    
    glTexCoord2f(1, 0);
    glVertex2f(1, -1);
    
    glTexCoord2f(1, 1);
    glVertex2f(1, 1);
    
    glTexCoord2f(0, 1);
    glVertex2f(-1, 1);
    
    glEnd();
    
    glBindTexture(GL_TEXTURE_2D, 0);
   }break;
  }
 }
 
 //- instance draw circles
 {
  GLuint QuadVBO = OpenGL->PerfectCircle.QuadVBO;
  GLuint CircleVBO = OpenGL->PerfectCircle.CircleVBO;
  perfect_circle_program *Prog = &OpenGL->PerfectCircle.Program;
  
  GL_CALL(OpenGL->glUseProgram(Prog->ProgramHandle));
  GL_CALL(OpenGL->glUniformMatrix3fv(Prog->Projection_UniformLoc,
                                     1, GL_TRUE, Cast(f32 *)Proj3x3.M));
  for (u32 AttrLocIndex = 0;
       AttrLocIndex < ArrayCount(Prog->Attributes.All);
       ++AttrLocIndex)
  {
   GLuint Attr = Prog->Attributes.All[AttrLocIndex];
   GL_CALL(OpenGL->glEnableVertexAttribArray(Attr));
  }
  
  GL_CALL(OpenGL->glBindBuffer(GL_ARRAY_BUFFER, QuadVBO));
  GL_CALL(OpenGL->glVertexAttribPointer(Prog->Attributes.VertP_AttrLoc, 2, GL_FLOAT, GL_FALSE, SizeOf(v2), 0));
  
  //- upload instances' data into GPU
  GL_CALL(OpenGL->glBindBuffer(GL_ARRAY_BUFFER, CircleVBO));
  GL_CALL(OpenGL->glBufferData(GL_ARRAY_BUFFER,
                               Frame->CircleCount * SizeOf(Frame->Circles[0]),
                               Frame->Circles,
                               GL_DYNAMIC_DRAW));
  
#define OpenGLInstancedVertexAttrib(Id, Member) \
GL_CALL(OpenGL->glVertexAttribPointer(Id, SizeOf((Cast(render_circle *)0)->Member)/SizeOf(f32), GL_FLOAT, GL_FALSE, SizeOf(render_circle), Cast(void *)OffsetOf(render_circle, Member))); \
GL_CALL(OpenGL->glVertexAttribDivisor(Id, 1))
  OpenGLInstancedVertexAttrib(Prog->Attributes.VertZ_AttrLoc, Z);
  OpenGLInstancedVertexAttrib(Prog->Attributes.VertModel0_AttrLoc, Model.Rows[0]);
  OpenGLInstancedVertexAttrib(Prog->Attributes.VertModel1_AttrLoc, Model.Rows[1]);
  OpenGLInstancedVertexAttrib(Prog->Attributes.VertModel2_AttrLoc, Model.Rows[2]);
  OpenGLInstancedVertexAttrib(Prog->Attributes.VertRadiusProper_AttrLoc, RadiusProper);
  OpenGLInstancedVertexAttrib(Prog->Attributes.VertColor_AttrLoc, Color);
  OpenGLInstancedVertexAttrib(Prog->Attributes.VertOutlineColor_AttrLoc, OutlineColor);
#undef OpenGLInstancedVertexAttrib
  
  GL_CALL(OpenGL->glDrawArraysInstanced(GL_QUADS, 0, 4, Frame->CircleCount));
  
  for (u32 AttrLocIndex = 0;
       AttrLocIndex < ArrayCount(Prog->Attributes.All);
       ++AttrLocIndex)
  {
   GLuint Attr = Prog->Attributes.All[AttrLocIndex];
   GL_CALL(OpenGL->glDisableVertexAttribArray(Attr));
  }
  GL_CALL(OpenGL->glUseProgram(0));
 }
 
 Memory->ImGuiBindings.Render();
 SwapBuffers(OpenGL->WindowDC);
}