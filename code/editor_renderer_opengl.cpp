global b32 GlobalRendererCodeReloadedOrRendererInitialized;

#define GL_NUM_EXTENSIONS                 0x821D

#define GL_MAX_COLOR_ATTACHMENTS            0x8CDF
#define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS 0x8B4D
#define GL_MAX_TEXTURE_IMAGE_UNITS          0x8872
#define GL_MAX_SAMPLES                    0x8D57
#define GL_MAX_COLOR_TEXTURE_SAMPLES      0x910E
#define GL_MAX_DEPTH_TEXTURE_SAMPLES      0x910F

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

#define GL_INVALID_FRAMEBUFFER_OPERATION  0x0506

#define GL_CALL(Expr) Expr; OpenGLCheckErrors()
#define GL_MAYBE_EXPECT_ERROR(Expr) Expr; OpenGLClearErrors()

#define GLFloatAttribPointerAndDivisor(OpenGL, Id, Type, Member, Divisor) \
GL_CALL(OpenGL->glVertexAttribPointer(Id, SizeOf(MemberOf(Type, Member))/SizeOf(f32), GL_FLOAT, GL_FALSE, SizeOf(Type), Cast(void *)OffsetOf(Type, Member))); \
GL_CALL(OpenGL->glVertexAttribDivisor(Id, Divisor))

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
 }
}

internal void
OpenGLClearErrors(void)
{
 while (glGetError() != GL_NO_ERROR);
}

GL_DEBUG_CALLBACK(OpenGLDebugCallback)
{
 MarkUnused(source);
 MarkUnused(type);
 MarkUnused(id);
 MarkUnused(severity);
 MarkUnused(length);
 MarkUnused(message);
 MarkUnused(userParam);
 
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

internal GLuint
OpenGLCreateProgram(opengl *OpenGL, char const *VertexCode, char const *FragmentCode)
{
 char const *ShaderCodeHeader = R"FOO(
 #version 330
 
#define i32 int
#define u32 uint
  #define f32 float
  #define v2 vec2
  #define v3 vec3
  #define v4 vec4
  #define v2i ivec2
  #define v3s ivec3
  #define v4s ivec4
#define V2 vec2
#define V3 vec3
#define V4 vec4
#define Lerp(A, B, T) mix(A, B, T)
  #define Square(X) ((X)*(X))
  #define Dot(U, V) dot(U, V)
  #define Length(V) length(V)
  #define Length2(V) dot(V, V)
#define Max(A, B) max(A, B)
#define Min(A, B) min(A, B)
#define Clamp01(T) clamp(T, 0, 1)
#define Clamp0Inf(T) max(T, 0)
#define Clamp(Min, T, Max) clamp(T, Min, Max)
  )FOO";
 
 GL_CALL(GLuint VertexShaderID = OpenGL->glCreateShader(GL_VERTEX_SHADER));
 GLchar *VertexShaderCode[] =
 {
  Cast(char *)ShaderCodeHeader,
  Cast(char *)VertexCode,
 };
 GL_CALL(OpenGL->glShaderSource(VertexShaderID, ArrayCount(VertexShaderCode), VertexShaderCode, 0));
 GL_CALL(OpenGL->glCompileShader(VertexShaderID));
 
 GL_CALL(GLuint FragmentShaderID = OpenGL->glCreateShader(GL_FRAGMENT_SHADER));
 GLchar *FragmentShaderCode[] =
 {
  Cast(char *)ShaderCodeHeader,
  Cast(char *)FragmentCode,
 };
 GL_CALL(OpenGL->glShaderSource(FragmentShaderID, ArrayCount(FragmentShaderCode), FragmentShaderCode, 0));
 GL_CALL(OpenGL->glCompileShader(FragmentShaderID));
 
 GL_CALL(GLuint ProgramID = OpenGL->glCreateProgram());
 GL_CALL(OpenGL->glAttachShader(ProgramID, VertexShaderID));
 GL_CALL(OpenGL->glAttachShader(ProgramID, FragmentShaderID));
 GL_CALL(OpenGL->glLinkProgram(ProgramID));
 
 GL_CALL(OpenGL->glValidateProgram(ProgramID));
 GLint Linked = false;
 GL_CALL(OpenGL->glGetProgramiv(ProgramID, GL_LINK_STATUS, &Linked));
 if(!Linked)
 {
  GLsizei Ignored;
  char VertexErrors[4096];
  char FragmentErrors[4096];
  char ProgramErrors[4096];
  GL_CALL(OpenGL->glGetShaderInfoLog(VertexShaderID, sizeof(VertexErrors), &Ignored, VertexErrors));
  GL_CALL(OpenGL->glGetShaderInfoLog(FragmentShaderID, sizeof(FragmentErrors), &Ignored, FragmentErrors));
  GL_CALL(OpenGL->glGetProgramInfoLog(ProgramID, sizeof(ProgramErrors), &Ignored, ProgramErrors));
  
  Assert(!"Shader validation failed");
 }
 
 GL_CALL(OpenGL->glDeleteShader(VertexShaderID));
 GL_CALL(OpenGL->glDeleteShader(FragmentShaderID));
 
 return ProgramID;
}

internal GLuint
CompileProgramCommon(opengl *OpenGL,
                     char const *VertexCode, char const *FragmentCode,
                     GLuint *Attributes, u32 AttributeCount, char const **AttributeNames,
                     GLuint *Uniforms, u32 UniformCount, char const **UniformNames)
{
 GLuint ProgramHandle = OpenGLCreateProgram(OpenGL, VertexCode, FragmentCode);
 
 for (u32 AttrIndex = 0;
      AttrIndex < AttributeCount;
      ++AttrIndex)
 {
  char const *AttributeName = AttributeNames[AttrIndex];
  GL_CALL(GLuint Attr = OpenGL->glGetAttribLocation(ProgramHandle, AttributeName));
  Attributes[AttrIndex] = Attr;
  //Assert(Attr != -1);
 }
 
 for (u32 UniformIndex = 0;
      UniformIndex < UniformCount;
      ++UniformIndex)
 {
  char const *UniformName = UniformNames[UniformIndex];
  GL_CALL(GLuint Uniform = OpenGL->glGetUniformLocation(ProgramHandle, UniformName));
  Uniforms[UniformIndex] = Uniform;
  //Assert(Uniform != -1);
 }
 
 return ProgramHandle;
}

internal line_program
CompileLineProgram(opengl *OpenGL)
{
 char const *VertexShader = R"FOO(
in v2 VertP;

out v4 FragColor;

uniform f32 Z;
uniform v4 Color;
uniform mat3 Transform;

void main(void) {
v3 P = Transform * v3(VertP, 1);
gl_Position = V4(P.xy, Z, P.z);
FragColor = Color;
}
)FOO";
 
 char const *FragmentShader = R"FOO(
in v4 FragColor;

out v4 OutColor;

void main(void) {
OutColor = FragColor;
}
)FOO";
 
 char const *AttributeNames[] =
 {
  "VertP"
 };
 char const *UniformNames[] =
 {
  "Z",
  "Color",
  "Transform",
 };
 StaticAssert(ArrayCount(AttributeNames) ==
              ArrayCount(MemberOf(line_program, Attributes.All)),
              AllAttributeNamesDefined);
 StaticAssert(ArrayCount(UniformNames) ==
              ArrayCount(MemberOf(line_program, Uniforms.All)),
              AllUniformNamesDefined);
 
 line_program Result = {};
 Result.ProgramHandle =
  CompileProgramCommon(OpenGL, VertexShader, FragmentShader,
                       Result.Attributes.All, ArrayCount(Result.Attributes.All), AttributeNames,
                       Result.Uniforms.All, ArrayCount(Result.Uniforms.All), UniformNames);
 
 return Result;
}

internal void
UseProgramBegin(opengl *OpenGL, perfect_circle_program *Prog, mat3 Proj)
{
 GL_CALL(OpenGL->glUseProgram(Prog->ProgramHandle));
 GL_CALL(OpenGL->glUniformMatrix3fv(Prog->Uniforms.Projection_UniformLoc,
                                    1, GL_TRUE, Cast(f32 *)Proj.M));
 for (u32 AttrLocIndex = 0;
      AttrLocIndex < ArrayCount(Prog->Attributes.All);
      ++AttrLocIndex)
 {
  GLuint Attr = Prog->Attributes.All[AttrLocIndex];
  GL_CALL(OpenGL->glEnableVertexAttribArray(Attr));
 }
}

internal void
UseProgramEnd(opengl *OpenGL, perfect_circle_program *Prog)
{
 for (u32 AttrLocIndex = 0;
      AttrLocIndex < ArrayCount(Prog->Attributes.All);
      ++AttrLocIndex)
 {
  GLuint Attr = Prog->Attributes.All[AttrLocIndex];
  GL_CALL(OpenGL->glDisableVertexAttribArray(Attr));
 }
 GL_CALL(OpenGL->glUseProgram(0));
}

internal void
UseProgramBegin(opengl *OpenGL, line_program *Prog)
{
 GL_CALL(OpenGL->glUseProgram(Prog->ProgramHandle));
 
 for (u32 AttrLocIndex = 0;
      AttrLocIndex < ArrayCount(Prog->Attributes.All);
      ++AttrLocIndex)
 {
  GLuint Attr = Prog->Attributes.All[AttrLocIndex];
  GL_CALL(OpenGL->glEnableVertexAttribArray(Attr));
 }
}

internal void
UseProgramEnd(opengl *OpenGL, line_program *Prog)
{
 for (u32 AttrLocIndex = 0;
      AttrLocIndex < ArrayCount(Prog->Attributes.All);
      ++AttrLocIndex)
 {
  GLuint Attr = Prog->Attributes.All[AttrLocIndex];
  GL_CALL(OpenGL->glDisableVertexAttribArray(Attr));
 }
 GL_CALL(OpenGL->glUseProgram(0));
}

internal void
UseProgramBegin(opengl *OpenGL, image_program *Prog, mat3 Proj)
{
 GL_CALL(OpenGL->glUseProgram(Prog->ProgramHandle));
 GL_CALL(OpenGL->glUniformMatrix3fv(Prog->Uniforms.Projection_UniformLoc,
                                    1, GL_TRUE, Cast(f32 *)Proj.M));
 for (u32 AttrLocIndex = 0;
      AttrLocIndex < ArrayCount(Prog->Attributes.All);
      ++AttrLocIndex)
 {
  GLuint Attr = Prog->Attributes.All[AttrLocIndex];
  GL_CALL(OpenGL->glEnableVertexAttribArray(Attr));
 }
}

internal void
UseProgramEnd(opengl *OpenGL, image_program *Prog)
{
 for (u32 AttrLocIndex = 0;
      AttrLocIndex < ArrayCount(Prog->Attributes.All);
      ++AttrLocIndex)
 {
  GLuint Attr = Prog->Attributes.All[AttrLocIndex];
  GL_CALL(OpenGL->glDisableVertexAttribArray(Attr));
 }
 GL_CALL(OpenGL->glUseProgram(0));
}

internal void
UseProgramBegin(opengl *OpenGL, vertex_program *Prog, mat3 Proj)
{
 GL_CALL(OpenGL->glUseProgram(Prog->ProgramHandle));
 GL_CALL(OpenGL->glUniformMatrix3fv(Prog->Uniforms.Projection_UniformLoc,
                                    1, GL_TRUE, Cast(f32 *)Proj.M));
 for (u32 AttrLocIndex = 0;
      AttrLocIndex < ArrayCount(Prog->Attributes.All);
      ++AttrLocIndex)
 {
  GLuint Attr = Prog->Attributes.All[AttrLocIndex];
  GL_CALL(OpenGL->glEnableVertexAttribArray(Attr));
 }
}

internal void
UseProgramEnd(opengl *OpenGL, vertex_program *Prog)
{
 for (u32 AttrLocIndex = 0;
      AttrLocIndex < ArrayCount(Prog->Attributes.All);
      ++AttrLocIndex)
 {
  GLuint Attr = Prog->Attributes.All[AttrLocIndex];
  GL_CALL(OpenGL->glDisableVertexAttribArray(Attr));
 }
 GL_CALL(OpenGL->glUseProgram(0));
}

internal perfect_circle_program
CompilePerfectCircleProgram(opengl *OpenGL)
{
 perfect_circle_program Result = {};
 
 char const *VertexShader = R"FOO(
in v2 VertP;
in f32 VertZ;
in mat3 VertModel;
in f32 VertRadiusProper;
in v4 VertColor;
in v4 VertOutlineColor;

out v2 FragP;
flat out f32 FragRadiusProper;
flat out v4 FragColor;
flat out v4 FragOutlineColor;

uniform mat3 Projection;

void main(void)
 {
mat3 Transform = Projection * VertModel;
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
f32 Dist = Length(FragP);
// NOTE(hbr): This is a really really nice function. fwidth returns derivative
// basically. GLSL, OpenGL or whatever calculates how "Dist" variable changes
// compared to the fragments (pixels) that are around the currently computed pixel.
// This let's us have resolution independent antialiasing. 1.6f parameter has been
// chosen experimentally - too high causes too much blur, too low causes sharp edges.
f32 Delta = 1.6f * fwidth(Dist);

f32 ProperEdge = Clamp0Inf(FragRadiusProper - (FragRadiusProper < 1 ? Delta : 0));
f32 ProperT = smoothstep(ProperEdge, FragRadiusProper, Dist);
f32 OutlineEdge = Clamp0Inf(1 - Delta);
f32 OutlineT = smoothstep(OutlineEdge, 1.0f, Dist);

v4 ProperColor = Lerp(FragColor, FragOutlineColor, ProperT);
f32 AlphaChannel = Lerp(ProperColor.a, 0, OutlineT);
 OutColor = V4(ProperColor.xyz, AlphaChannel);

//OutColor = V4(AlphaChannel, AlphaChannel, AlphaChannel, AlphaChannel);
 }
 )FOO";
 
 GLuint ProgramHandle = OpenGLCreateProgram(OpenGL, VertexShader, FragmentShader);
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
 StaticAssert(ArrayCount(AttrNames) ==
              ArrayCount(MemberOf(perfect_circle_program, Attributes.All)),
              AttrNamesLengthMatchesDefinedAttributes);
 
 char const *UniformNames[] =
 {
  "Projection",
 };
 StaticAssert(ArrayCount(UniformNames) ==
              ArrayCount(MemberOf(perfect_circle_program, Uniforms.All)),
              UniformNamesLengthMatchesDefinedUniforms);
 
 Result.ProgramHandle =
  CompileProgramCommon(OpenGL,
                       VertexShader, FragmentShader,
                       Result.Attributes.All, ArrayCount(Result.Attributes.All), AttrNames,
                       Result.Uniforms.All, ArrayCount(Result.Uniforms.All), UniformNames);
 
 // NOTE(hbr): need to fix two rows of mat3 attribute
 Result.Attributes.VertModel1_AttrLoc = Result.Attributes.VertModel0_AttrLoc + 1;
 Result.Attributes.VertModel2_AttrLoc = Result.Attributes.VertModel0_AttrLoc + 2;
 
 return Result;
}

internal image_program
CompileImageProgram(opengl *OpenGL)
{
 char const *VertexShader = R"FOO(
in v2 VertP;
in v2 VertUV;
in f32 VertZ;
in mat3 VertModel;

flat out i32 FragTextureHandle;
  out v2 FragUV;

uniform mat3 Projection;

void main(void) {
v3 P = Projection * VertModel * v3(VertP, 1);
gl_Position = V4(P.xy, VertZ, P.z);
FragTextureHandle = gl_InstanceID;
FragUV = VertUV;
}
)FOO";
 
 char const *FragmentShader = R"FOO(
flat in i32 FragTextureHandle;
  in v2 FragUV;

out v4 OutColor;

uniform sampler2D Sampler0;
uniform sampler2D Sampler1;
uniform sampler2D Sampler2;
uniform sampler2D Sampler3;
uniform sampler2D Sampler4;
uniform sampler2D Sampler5;
uniform sampler2D Sampler6;
uniform sampler2D Sampler7;

void main(void) {
switch (FragTextureHandle) {
case 0: {OutColor = texture(Sampler0, FragUV);} break;
case 1: {OutColor = texture(Sampler1, FragUV);} break;
case 2: {OutColor = texture(Sampler2, FragUV);} break;
case 3: {OutColor = texture(Sampler3, FragUV);} break;
case 4: {OutColor = texture(Sampler4, FragUV);} break;
case 5: {OutColor = texture(Sampler5, FragUV);} break;
case 6: {OutColor = texture(Sampler6, FragUV);} break;
case 7: {OutColor = texture(Sampler7, FragUV);} break;
}
}

)FOO";
 
 char const *AttributeNames[] =
 {
  "VertP",
  "VertUV",
  "VertZ",
  "VertModel",
  "VertModel",
  "VertModel",
 };
 char const *UniformNames[] =
 {
  "Projection",
  "Sampler0",
  "Sampler1",
  "Sampler2",
  "Sampler3",
  "Sampler4",
  "Sampler5",
  "Sampler6",
  "Sampler7",
 };
 StaticAssert(ArrayCount(AttributeNames) ==
              ArrayCount(MemberOf(image_program, Attributes.All)),
              AllAttributeNamesDefined);
 StaticAssert(ArrayCount(UniformNames) ==
              ArrayCount(MemberOf(image_program, Uniforms.All)),
              AllUniformNamesDefined);
 
 image_program Result = {};
 Result.ProgramHandle =
  CompileProgramCommon(OpenGL, VertexShader, FragmentShader,
                       Result.Attributes.All, ArrayCount(Result.Attributes.All), AttributeNames,
                       Result.Uniforms.All, ArrayCount(Result.Uniforms.All), UniformNames);
 Result.Attributes.VertModel0_AttrLoc = Result.Attributes.VertModel0_AttrLoc + 0;
 Result.Attributes.VertModel1_AttrLoc = Result.Attributes.VertModel0_AttrLoc + 1;
 Result.Attributes.VertModel2_AttrLoc = Result.Attributes.VertModel0_AttrLoc + 2;
 
 return Result;
}

internal vertex_program
CompileVertexProgram(opengl *OpenGL)
{
 char const *VertexShader = R"FOO(
in v2 VertP;
in v4 VertColor;
in f32 VertZ;

out v4 FragColor;

uniform mat3 Projection;

void main(void) {
v3 P = Projection * V3(VertP, 1);
gl_Position = V4(P.xy, VertZ, P.z);
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
 
 char const *AttributeNames[] =
 {
  "VertP",
  "VertZ",
  "VertColor",
 };
 char const *UniformNames[] =
 {
  "Projection",
 };
 StaticAssert(ArrayCount(AttributeNames) ==
              ArrayCount(MemberOf(vertex_program, Attributes.All)),
              AllAttributeNamesDefined);
 StaticAssert(ArrayCount(UniformNames) ==
              ArrayCount(MemberOf(vertex_program, Uniforms.All)),
              AllUniformNamesDefined);
 
 vertex_program Result = {};
 Result.ProgramHandle =
  CompileProgramCommon(OpenGL, VertexShader, FragmentShader,
                       Result.Attributes.All, ArrayCount(Result.Attributes.All), AttributeNames,
                       Result.Uniforms.All, ArrayCount(Result.Uniforms.All), UniformNames);
 
 return Result;
}

internal void
OpenGLInit(opengl *OpenGL, arena *Arena, renderer_memory *Memory)
{
 GL_CALL(glEnable(GL_BLEND));
 GL_CALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
 GL_CALL(glEnable(GL_MULTISAMPLE));
 // NOTE(hbr): So that glEnable,glEnd with glBindTexture works. When using shaders it is not needed.
 GL_CALL(glEnable(GL_TEXTURE_2D));
 GL_CALL(glEnable(GL_DEPTH_TEST));
 // TODO(hbr): For now I set this to always so that transparency somehow works.
 // I think I need to implement depth peeling (or sorting but this can be ugly).
 GL_CALL(glDepthFunc(GL_ALWAYS));
 
 GL_CALL(glEnable(GL_DEBUG_OUTPUT));
 GL_CALL(glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS));
 
 GLuint DummyVAO;
 GL_CALL(OpenGL->glGenVertexArrays(1, &DummyVAO));
 GL_CALL(OpenGL->glBindVertexArray(DummyVAO));
 
 OpenGL->RenderFrame.Arena = AllocArena(Gigabytes(64));
 
 //- allocate texutre indices
 {
  u32 TextureCount = Memory->Limits.MaxTextureCount + 1;
  GLuint *Textures = PushArray(Arena, TextureCount, GLuint);
  OpenGL->MaxTextureCount = TextureCount;
  OpenGL->Textures = Textures;
  
  GL_CALL(glGenTextures(Cast(GLsizei)TextureCount, Textures));
  for (u32 TextureIndex = 0;
       TextureIndex < TextureCount;
       ++TextureIndex)
  {
   GL_CALL(glBindTexture(GL_TEXTURE_2D, Textures[TextureIndex]));
   if (TextureIndex == 0)
   {
    unsigned char WhitePixel[4] = { 255, 255, 255, 255 };
    GL_CALL(glTexImage2D(GL_TEXTURE_2D,
                         0,
                         GL_RGBA,
                         1, 1, 0,
                         GL_RGBA,
                         GL_UNSIGNED_BYTE,
                         WhitePixel));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
   }
   else
   {
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
   }
   GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP));
   GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP));
   GL_CALL(glBindTexture(GL_TEXTURE_2D, 0));
  }
 }
 
 //- allocate buffer indices
 {
  u32 BufferCount = Memory->Limits.MaxBufferCount;
  GLuint *Buffers = PushArray(Arena, BufferCount, GLuint);
  GL_CALL(OpenGL->glGenBuffers(Cast(GLsizei)BufferCount, Buffers));
  OpenGL->MaxBufferCount = BufferCount;
  OpenGL->Buffers = Buffers;
 }
 
 //- allocate buffers
 {
  GL_CALL(OpenGL->glGenBuffers(1, &OpenGL->PerfectCircle.QuadVBO));
  GL_CALL(OpenGL->glGenBuffers(1, &OpenGL->PerfectCircle.CircleVBO));
  GL_CALL(OpenGL->glGenBuffers(1, &OpenGL->Line.VertexBuffer));
  GL_CALL(OpenGL->glGenBuffers(1, &OpenGL->Image.VertexBuffer));
  GL_CALL(OpenGL->glGenBuffers(1, &OpenGL->Image.ImageBuffer));
  GL_CALL(OpenGL->glGenBuffers(1, &OpenGL->Vertex.VertexBuffer));
  
  {
   v2 Vertices[] =
   {
    V2(-1, -1),
    V2( 1, -1),
    V2( 1,  1),
    
    V2(-1, -1),
    V2( 1,  1),
    V2(-1,  1),
   };
   GL_CALL(OpenGL->glBindBuffer(GL_ARRAY_BUFFER, OpenGL->PerfectCircle.QuadVBO));
   GL_CALL(OpenGL->glBufferData(GL_ARRAY_BUFFER, SizeOf(Vertices), Vertices, GL_STATIC_DRAW));
  }
  
  {
   image_vertex Vertices[] =
   {
    { V2(-1, -1), V2(0, 0) },
    { V2( 1, -1), V2(1, 0) },
    { V2( 1,  1), V2(1, 1) },
    
    { V2(-1, -1), V2(0, 0) },
    { V2( 1,  1), V2(1, 1) },
    { V2(-1,  1), V2(0, 1) },
   };
   GL_CALL(OpenGL->glBindBuffer(GL_ARRAY_BUFFER, OpenGL->Image.VertexBuffer));
   GL_CALL(OpenGL->glBufferData(GL_ARRAY_BUFFER, SizeOf(Vertices), Vertices, GL_STATIC_DRAW));
  }
 }
 
 {
  GLint MaxTextureSlots;
  GL_CALL(glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &MaxTextureSlots));
  OpenGL->MaxTextureSlots = 1;
  if (MaxTextureSlots > 0) OpenGL->MaxTextureSlots = MaxTextureSlots;
 }
 
 GlobalRendererCodeReloadedOrRendererInitialized = true;
}

internal render_frame *
OpenGLBeginFrame(opengl *OpenGL, renderer_memory *Memory, v2u WindowDim)
{
 ProfileFunctionBegin();
 
 render_frame *RenderFrame = &OpenGL->RenderFrame;
 RenderFrame->PolygonModeIsWireFrame = OpenGL->PolygonModeIsWireFrame;
 
 ClearArena(RenderFrame->Arena);
 
 if (GlobalRendererCodeReloadedOrRendererInitialized)
 {
  ProfileBegin("HotReloadShaders");
  
  //- fix error handling function pointer
  if (OpenGL->glDebugMessageCallback)
  {
   //GL_CALL(OpenGL->glDebugMessageCallback(OpenGLDebugCallback, 0));
  }
  
  //- recompile shaders
  GL_MAYBE_EXPECT_ERROR(OpenGL->glDeleteProgram(OpenGL->PerfectCircle.Program.ProgramHandle));
  OpenGL->PerfectCircle.Program = CompilePerfectCircleProgram(OpenGL);
  
  GL_MAYBE_EXPECT_ERROR(OpenGL->glDeleteProgram(OpenGL->Line.Program.ProgramHandle));
  OpenGL->Line.Program = CompileLineProgram(OpenGL);
  
  GL_MAYBE_EXPECT_ERROR(OpenGL->glDeleteProgram(OpenGL->Image.Program.ProgramHandle));
  OpenGL->Image.Program = CompileImageProgram(OpenGL);
  
  GL_MAYBE_EXPECT_ERROR(OpenGL->glDeleteProgram(OpenGL->Vertex.Program.ProgramHandle));
  OpenGL->Vertex.Program = CompileVertexProgram(OpenGL);
  
  GlobalRendererCodeReloadedOrRendererInitialized = false;
  
  ProfileEnd();
 }
 
 RenderFrame->LineCount = 0;
 RenderFrame->Lines = Memory->LineBuffer;
 RenderFrame->MaxLineCount = Memory->MaxLineCount;
 RenderFrame->CircleCount = 0;
 RenderFrame->Circles = Memory->CircleBuffer;
 RenderFrame->MaxCircleCount = Memory->MaxCircleCount;
 RenderFrame->ImageCount = 0;
 RenderFrame->Images = Memory->ImageBuffer;
 RenderFrame->MaxImageCount = Memory->MaxImageCount;
 RenderFrame->VertexCount = 0;
 RenderFrame->Vertices = Memory->VertexBuffer;
 RenderFrame->MaxVertexCount = Memory->MaxVertexCount;
 RenderFrame->WindowDim = WindowDim;
 
 ProfileBlock("ImGuiNewFrame")
 { 
  Memory->ImGuiNewFrame();
 }
 
 ProfileEnd();
 
 return RenderFrame;
}

internal void
OpenGLManageTransferQueue(opengl *OpenGL, renderer_transfer_queue *Queue)
{
 ProfileFunctionBegin();
 
 GLuint *Textures = OpenGL->Textures;
 
 while (Queue->OpCount)
 {
  renderer_transfer_op *Op = Queue->Ops + Queue->FirstOpIndex;
  
  if (Op->State == RendererOp_ReadyToTransfer)
  {
   switch (Op->Type)
   {
    case RendererTransferOp_Texture: {
     u32 TextureIndex = TextureIndexFromHandle(Op->TextureHandle);
     Assert(TextureIndex < OpenGL->MaxTextureCount);
     GL_CALL(glBindTexture(GL_TEXTURE_2D, Textures[TextureIndex]));
     GL_CALL(glTexImage2D(GL_TEXTURE_2D,
                          0,
                          GL_RGBA,
                          Cast(u32)Op->Width,
                          Cast(u32)Op->Height,
                          0,
                          GL_RGBA,
                          GL_UNSIGNED_BYTE,
                          Op->Pixels));
     GL_CALL(OpenGL->glGenerateMipmap(GL_TEXTURE_2D));
     GL_CALL(glBindTexture(GL_TEXTURE_2D, 0));
    }break;
    
    case RendererTransferOp_Buffer: {
     NotImplemented;
    }break;
   }
  }
  else if (Op->State == RendererOp_PendingLoad)
  {
   break;
  }
  else
  {
   Assert(Op->State == RendererOp_Empty);
  }
  
  Queue->FreeOffset = Op->SavedAllocateOffset;
  --Queue->OpCount;
  ++Queue->FirstOpIndex;
  if (Queue->FirstOpIndex == MAX_RENDERER_TRANFER_QUEUE_COUNT)
  {
   Queue->FirstOpIndex = 0;
  }
 }
 
 ProfileEnd();
}

internal void
OpenGLEndFrame(opengl *OpenGL, renderer_memory *Memory, render_frame *Frame)
{
 ProfileFunctionBegin();
 
 if (!!OpenGL->PolygonModeIsWireFrame != !!Frame->PolygonModeIsWireFrame)
 {
  OpenGL->PolygonModeIsWireFrame = Frame->PolygonModeIsWireFrame;
  if (OpenGL->PolygonModeIsWireFrame)
  {
   GL_CALL(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));
  }
  else
  {
   GL_CALL(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
  }
 }
 
 renderer_transfer_queue *Queue = &Memory->RendererQueue;
 GLuint *Textures = OpenGL->Textures;
 mat3 Projection = Frame->Proj;
 
 rgba Clear = Frame->ClearColor;
 GL_CALL(glClearColor(Clear.R, Clear.G, Clear.B, Clear.A));
 GL_CALL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
 
 GL_CALL(glViewport(0, 0, Frame->WindowDim.X, Frame->WindowDim.Y));
 
 OpenGLManageTransferQueue(OpenGL, Queue);
 
 //- draw images
 {
  ProfileBegin("DrawImages");
  
  image_program *Prog = &OpenGL->Image.Program;
  UseProgramBegin(OpenGL, Prog, Projection);
  
  GL_CALL(OpenGL->glBindBuffer(GL_ARRAY_BUFFER, OpenGL->Image.VertexBuffer));
  GLFloatAttribPointerAndDivisor(OpenGL, Prog->Attributes.VertP_AttrLoc, image_vertex, P, 0);
  GLFloatAttribPointerAndDivisor(OpenGL, Prog->Attributes.VertUV_AttrLoc, image_vertex, UV, 0);
  
  GL_CALL(OpenGL->glBindBuffer(GL_ARRAY_BUFFER, OpenGL->Image.ImageBuffer));
  GL_CALL(OpenGL->glBufferData(GL_ARRAY_BUFFER,
                               Frame->ImageCount * SizeOf(render_image),
                               Frame->Images, GL_DYNAMIC_DRAW));
  GLFloatAttribPointerAndDivisor(OpenGL, Prog->Attributes.VertZ_AttrLoc, render_image, Z, 1);
  GLFloatAttribPointerAndDivisor(OpenGL, Prog->Attributes.VertModel0_AttrLoc, render_image, Model.M.Rows[0], 1);
  GLFloatAttribPointerAndDivisor(OpenGL, Prog->Attributes.VertModel1_AttrLoc, render_image, Model.M.Rows[1], 1);
  GLFloatAttribPointerAndDivisor(OpenGL, Prog->Attributes.VertModel2_AttrLoc, render_image, Model.M.Rows[2], 1);
  
  u32 ImagesLeft = Frame->ImageCount;
  render_image *ImageAt = Frame->Images;
  u32 ImageOffset = 0;
  while (ImagesLeft)
  {
   u32 ImageCount = ImagesLeft;
   ImageCount = Min(ImageCount, OpenGL->MaxTextureSlots);
   ImageCount = Min(ImageCount, ArrayCount(MemberOf(image_program, Uniforms.Samplers)));
   
   for (u32 SlotIndex = 0;
        SlotIndex < ImageCount;
        ++SlotIndex)
   {
    render_image *Image = ImageAt + SlotIndex;
    u32 TextureIndex = TextureIndexFromHandle(Image->TextureHandle);
    GLuint TextureID = Textures[TextureIndex];
    
    GL_CALL(OpenGL->glActiveTexture(GL_TEXTURE0 + SlotIndex));
    GL_CALL(glBindTexture(GL_TEXTURE_2D, TextureID));
    GL_CALL(OpenGL->glUniform1i(Prog->Uniforms.Samplers[SlotIndex], SlotIndex));
   }
   
   // TODO(hbr): draw elements
   GL_CALL(OpenGL->glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 6, ImageCount, ImageOffset));
   
   ImageAt += ImageCount;
   ImagesLeft -= ImageCount;
   ImageOffset += ImageCount;
  }
  
  UseProgramEnd(OpenGL, Prog);
  
  ProfileEnd();
 }
 
 //- draw lines
 {
  ProfileBegin("DrawLines");
  
  line_program *Prog = &OpenGL->Line.Program;
  UseProgramBegin(OpenGL, Prog);
  
  for (u32 LineIndex = 0;
       LineIndex < Frame->LineCount;
       ++LineIndex)
  {
   render_line *Line = Frame->Lines + LineIndex;
   mat3 Transform = Projection * Line->Model.M;
   
   GL_CALL(OpenGL->glUniform1f(Prog->Uniforms.Z_UniformLoc, Line->ZOffset));
   GL_CALL(OpenGL->glUniform4fv(Prog->Uniforms.Color_UniformLoc, 1, Cast(f32 *)Line->Color.C.E));
   GL_CALL(OpenGL->glUniformMatrix3fv(Prog->Uniforms.Transform_UniformLoc, 1, GL_TRUE, Cast(f32 *)Transform.M));
   
   GLint glPrimitive = 0;
   switch (Line->Primitive)
   {
    case Primitive_TriangleStrip: {glPrimitive = GL_TRIANGLE_STRIP;}break;
    case Primitive_Triangles:     {glPrimitive = GL_TRIANGLES;}break;
   }
   
   GL_CALL(OpenGL->glBindBuffer(GL_ARRAY_BUFFER, OpenGL->Line.VertexBuffer));
   GL_CALL(OpenGL->glBufferData(GL_ARRAY_BUFFER,
                                Line->VertexCount * SizeOf(Line->Vertices[0]),
                                Line->Vertices,
                                GL_DYNAMIC_DRAW));
   
   GLFloatAttribPointerAndDivisor(OpenGL, Prog->Attributes.VertP_AttrLoc, v2, E, 0);
   
   GL_CALL(OpenGL->glDrawArrays(glPrimitive, 0, Line->VertexCount));
  }
  
  UseProgramEnd(OpenGL, Prog);
  
  ProfileEnd();
 }
 
 //- draw vertices
 {
  ProfileBegin("DrawVertices");
  
  vertex_program *Prog = &OpenGL->Vertex.Program;
  UseProgramBegin(OpenGL, Prog, Projection);
  
  GL_CALL(OpenGL->glBindBuffer(GL_ARRAY_BUFFER, OpenGL->Vertex.VertexBuffer));
  GL_CALL(OpenGL->glBufferData(GL_ARRAY_BUFFER,
                               Frame->VertexCount * SizeOf(render_vertex),
                               Frame->Vertices,
                               GL_DYNAMIC_DRAW));
  
  GLFloatAttribPointerAndDivisor(OpenGL, Prog->Attributes.VertP_AttrLoc, render_vertex, P, 0);
  GLFloatAttribPointerAndDivisor(OpenGL, Prog->Attributes.VertZ_AttrLoc, render_vertex, Z, 0);
  GLFloatAttribPointerAndDivisor(OpenGL, Prog->Attributes.VertColor_AttrLoc, render_vertex, Color, 0);
  
  GL_CALL(OpenGL->glDrawArrays(GL_TRIANGLES, 0, Frame->VertexCount));
  
  UseProgramEnd(OpenGL, Prog);
  
  ProfileEnd();
 }
 
 //- instance draw circles
 {
  ProfileBegin("InstaceDrawCircles");
  
  GLuint QuadVBO = OpenGL->PerfectCircle.QuadVBO;
  GLuint CircleVBO = OpenGL->PerfectCircle.CircleVBO;
  
  perfect_circle_program *Prog = &OpenGL->PerfectCircle.Program;
  UseProgramBegin(OpenGL, Prog, Projection);
  
  GL_CALL(OpenGL->glBindBuffer(GL_ARRAY_BUFFER, QuadVBO));
  GLFloatAttribPointerAndDivisor(OpenGL, Prog->Attributes.VertP_AttrLoc, v2, E, 0);
  
  GL_CALL(OpenGL->glBindBuffer(GL_ARRAY_BUFFER, CircleVBO));
  GL_CALL(OpenGL->glBufferData(GL_ARRAY_BUFFER,
                               Frame->CircleCount * SizeOf(Frame->Circles[0]),
                               Frame->Circles,
                               GL_DYNAMIC_DRAW));
  
  GLFloatAttribPointerAndDivisor(OpenGL, Prog->Attributes.VertZ_AttrLoc, render_circle, Z, 1);
  GLFloatAttribPointerAndDivisor(OpenGL, Prog->Attributes.VertModel0_AttrLoc, render_circle, Model.M.Rows[0], 1);
  GLFloatAttribPointerAndDivisor(OpenGL, Prog->Attributes.VertModel1_AttrLoc, render_circle, Model.M.Rows[1], 1);
  GLFloatAttribPointerAndDivisor(OpenGL, Prog->Attributes.VertModel2_AttrLoc, render_circle, Model.M.Rows[2], 1);
  GLFloatAttribPointerAndDivisor(OpenGL, Prog->Attributes.VertRadiusProper_AttrLoc, render_circle, RadiusProper, 1);
  GLFloatAttribPointerAndDivisor(OpenGL, Prog->Attributes.VertColor_AttrLoc, render_circle, Color, 1);
  GLFloatAttribPointerAndDivisor(OpenGL, Prog->Attributes.VertOutlineColor_AttrLoc, render_circle, OutlineColor, 1);
  
  // TODO(hbr): use draw elements
  GL_CALL(OpenGL->glDrawArraysInstanced(GL_TRIANGLES, 0, 6, Frame->CircleCount));
  
  UseProgramEnd(OpenGL, Prog);
  
  ProfileEnd();
 }
 
 ProfileBlock("ImGuiRender")
 {
  Memory->ImGuiRender();
 }
 
 ProfileEnd();
}
