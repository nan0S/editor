internal rgba
RGBA(f32 R, f32 G, f32 B, f32 A)
{
 rgba Result = {};
 Result.R = R;
 Result.G = G;
 Result.B = B;
 Result.A = A;
 return Result;
}

internal hsva
HSVA(f32 H, f32 S, f32 V, f32 A)
{
 hsva Result = {};
 Result.H = H;
 Result.S = S;
 Result.V = V;
 Result.A = A;
 return Result;
}

internal rgba
RGBA_U8(u8 R, u8 G, u8 B, u8 A)
{
 rgba Result = {};
 Result.R = R / 255.0f;
 Result.G = G / 255.0f;
 Result.B = B / 255.0f;
 Result.A = A / 255.0f;
 return Result;
}

internal rgba
RGBA_V4(v4 C)
{
 rgba Result = RGBA(C.R, C.G, C.B, C.A);
 return Result;
}

internal rgba
RGB_Hex(u32 HexCode)
{
 u8 B = (HexCode >> 0) & 0xff;
 u8 G = (HexCode >> 8) & 0xff;
 u8 R = (HexCode >> 16) & 0xff;
 Assert(((HexCode >> 24) & 0xff) == 0);
 rgba Result = RGBA_U8(R, G, B);
 return Result;
}

internal rgba
RGBA_From_HSVA(hsva Color)
{
 f32 H = Color.H;
 f32 S = Color.S;
 f32 V = Color.V;
 f32 A = Color.A;
 
 H = ModF32(H, 360.0f);
 if (H < 0.0f)
 {
  H += 360.0f;
 }
 
 f32 Chroma = V * S;
 f32 HueSection = H / 60.0f;
 f32 Secondary = Chroma * (1.0f - Abs(ModF32(HueSection, 2.0f) - 1.0f));
 f32 Match = V - Chroma;
 
 f32 R, G, B;
 if (HueSection < 1.0f)
 {
  R = Chroma;
  G = Secondary;
  B = 0.0f;
 }
 else if (HueSection < 2.0f)
 {
  R = Secondary;
  G = Chroma;
  B = 0.0f;
 }
 else if (HueSection < 3.0f)
 {
  R = 0.0f;
  G = Chroma;
  B = Secondary;
 } else if (HueSection < 4.0f)
 {
  R = 0.0f;
  G = Secondary;
  B = Chroma;
 }
 else if (HueSection < 5.0f)
 {
  R = Secondary;
  G = 0.0f;
  B = Chroma;
 }
 else
 {
  R = Chroma;
  G = 0.0f;
  B = Secondary;
 }
 
 rgba Result = RGBA(R + Match, G + Match, B + Match, A);
 return Result;
}

internal rgba
RGBA_Opposite(rgba RGB)
{
 hsva HSV = HSVA_From_RGBA(RGB);
 hsva HSV_Opposite = HSVA_Opposite(HSV);
 rgba Result = RGBA_From_HSVA(HSV_Opposite);
 return Result;
}

internal hsva
HSVA_From_RGBA(rgba Color)
{
 f32 R = Color.R;
 f32 G = Color.G;
 f32 B = Color.B;
 f32 A = Color.A;
 
 f32 Maxi = Max(R, Max(G, B));
 f32 Mini = Min(R, Min(G, B));
 f32 Delta = Maxi - Mini;
 
 f32 Hue = 0.0f;
 if (Delta == 0.0f)
 {
  Hue = 0.0f; // Undefined hue, conventionally set to 0
 }
 else if (Maxi == R)
 {
  Hue = 60.0f * ModF32(((G - B) / Delta), 6.0f);
  if (Hue < 0.0f) Hue += 360.0f;
 }
 else if (Maxi == G)
 {
  Hue = 60.0f * (((B - R) / Delta) + 2.0f);
 }
 else if (Maxi == B)
 {
  Hue = 60.0f * (((R - G) / Delta) + 4.0f);
 }
 
 f32 Saturation = (Maxi == 0.0f) ? 0.0f : (Delta / Maxi);
 f32 Value = Maxi;
 
 hsva Result = {};
 Result.H = Hue;
 Result.S = Saturation;
 Result.V = Value;
 Result.A = A;
 
 return Result;
}

internal rgba
RGBA_Gray(u8 Gray, u8 Alpha)
{
 rgba Result = RGBA_U8(Gray, Gray, Gray, Alpha);
 return Result;
}

internal rgba
RGBA_Brighten(rgba Color, f32 BrightenByRatio)
{
 rgba Result = {};
 Result.R = (1.0f + BrightenByRatio) * Color.R;
 Result.G = (1.0f + BrightenByRatio) * Color.G;
 Result.B = (1.0f + BrightenByRatio) * Color.B;
 Result.A = Color.A;
 
 return Result;
}

inline internal rgba
RGBA_Darken(rgba Color, f32 DarkenByRatio)
{
 rgba Result = RGBA_Brighten(Color, -DarkenByRatio);
 return Result;
}

inline internal rgba
RGBA_Fade(rgba Color, f32 FadeByRatio)
{
 rgba Result = Color;
 Result.A *= FadeByRatio;
 return Result;
}

internal hsva
HSVA_Opposite(hsva Color)
{
 hsva Result = Color;
 Result.H = ModF32(Result.H + 180.0f, 360.0f);
 return Result;
}

internal f32
Norm(v2 V)
{
 f32 Result = SqrtF32(NormSquared(V));
 return Result;
}

internal f32
NormSquared(v2 V)
{
 f32 Result = V.X*V.X + V.Y*V.Y;
 return Result;
}

internal i32
NormSquared(v2i V)
{
 i32 Result = V.X*V.X + V.Y*V.Y;
 return Result;
}

internal void
Normalize(v2 *V)
{
 f32 NormValue = Norm(*V);
 if (NormValue != 0.0f)
 {
  *V = *V / NormValue;
 }
}

internal v2
Normalized(v2 V)
{
 Normalize(&V);
 return V;
}

internal f32
Dot(v2 U, v2 V)
{
 f32 Result = U.X*V.X + U.Y*V.Y;
 return Result;
}

internal f32
Cross(v2 U, v2 V)
{
 f32 Result = U.X*V.Y - U.Y*V.X;
 return Result;
}

internal v2
Hadamard(v2 A, v2 B)
{
 v2 Result = {};
 Result.X = A.X * B.X;
 Result.Y = A.Y * B.Y;
 
 return Result;
}

internal v2
ProjectOnto(v2 U, v2 Onto)
{
 v2 Result = (Dot(U, Onto) / Dot(Onto, Onto)) * Onto;
 return Result;
}

inline internal rotation2d
Rotation2D(f32 X, f32 Y)
{
 rotation2d Rotation = Rotation2D(V2(X, Y));
 return Rotation;
}

inline internal rotation2d
Rotation2D(v2 V)
{
 rotation2d Rotation = {};
 Rotation.V = V;
 return Rotation;
}

inline internal rotation2d
Rotation2DZero(void)
{
 rotation2d Result = Rotation2D(1.0f, 0.0f);
 return Result;
}

internal rotation2d
Rotation2DFromVector(v2 Vector)
{
 Normalize(&Vector);
 rotation2d Result = Rotation2D(Vector.X, Vector.Y);
 
 return Result;
}

internal rotation2d
Rotation2DFromDegrees(f32 Degrees)
{
 f32 Radians = DegToRadF32 * Degrees;
 rotation2d Result = Rotation2DFromRadians(Radians);
 
 return Result;
}

internal rotation2d
Rotation2DFromRadians(f32 Radians)
{
 rotation2d Result = Rotation2D(CosF32(Radians), SinF32(Radians));
 return Result;
}

internal f32
Rotation2DToDegrees(rotation2d Rotation)
{
 f32 Radians = Rotation2DToRadians(Rotation);
 f32 Degrees = RadToDegF32 * Radians;
 
 return Degrees;
}

internal f32
Rotation2DToRadians(rotation2d Rotation)
{
 f32 Radians = Atan2F32(Rotation.V.Y, Rotation.V.X);
 return Radians;
}

internal rotation2d
Rotation2DInverse(rotation2d Rotation)
{
 rotation2d InverseRotation = Rotation2D(Rotation.V.X, -Rotation.V.Y);
 return InverseRotation;
}

internal rotation2d
Rotate90DegreesAntiClockwise(rotation2d Rotation)
{
 rotation2d Result = Rotation2D(-Rotation.V.Y, Rotation.V.X);
 return Result;
}

internal rotation2d
Rotate90DegreesClockwise(rotation2d Rotation)
{
 rotation2d Result = Rotation2D(Rotation.V.Y, -Rotation.V.X);
 return Result;
}

internal rotation2d
Rotation2DFromMovementAroundPoint(v2 From, v2 To, v2 Center)
{
 v2 FromTranslated = From - Center;
 v2 ToTranslated = To - Center;
 
 Normalize(&FromTranslated);
 Normalize(&ToTranslated);
 
 f32 Cos = Dot(FromTranslated, ToTranslated);
 f32 Sin = Cross(FromTranslated, ToTranslated);
 if (Cos == 0.0f && Sin == 0.0f)
 {
  Cos = 1.0f;
  Sin = 0.0f;
 }
 Assert(ApproxEq32(Cos*Cos + Sin*Sin, 1.0f));
 
 rotation2d Rotation = Rotation2D(Cos, Sin);
 return Rotation;
}

internal v2
RotateAround(v2 Point, v2 Center, rotation2d Rotation)
{
 v2 Translated = Point - Center;
 v2 Rotated = V2(Rotation.V.X * Translated.X - Rotation.V.Y * Translated.Y,
                 Rotation.V.X * Translated.Y + Rotation.V.Y * Translated.X);
 v2 Result = Rotated + Center;
 
 return Result;
}

internal rotation2d
CombineRotations2D(rotation2d A, rotation2d B)
{
 f32 RotationX = A.V.X * B.V.X - A.V.Y * B.V.Y;
 f32 RotationY = A.V.X * B.V.Y + A.V.Y * B.V.X;
 v2 RotationVector = V2(RotationX, RotationY);
 Normalize(&RotationVector);
 
 rotation2d Result = Rotation2D(RotationVector.X, RotationVector.Y);
 return Result;
}

inline internal scale2d
Scale2D(f32 X, f32 Y)
{
 scale2d Scale = {};
 Scale.V = V2(X, Y);
 return Scale;
}

inline internal scale2d
Scale2DOne(void)
{
 scale2d Scale = Scale2D(1, 1);
 return Scale;
}

inline internal xform2d
XForm2D(v2 P, scale2d Scale, rotation2d Rotation)
{
 xform2d XForm = {};
 XForm.P = P;
 XForm.Scale = Scale;
 XForm.Rotation = Rotation;
 return XForm;
}

inline internal xform2d
XForm2DZero(void)
{
 xform2d XForm = XForm2DFromP(V2(0, 0));
 return XForm;
}

inline internal xform2d
XForm2DFromP(v2 P)
{
 xform2d XForm = XForm2D(P, Scale2DOne(), Rotation2DZero());
 return XForm;
}

internal b32
AngleCompareLess(v2 A, v2 B, v2 O)
{
 v2 U = A - O;
 v2 V = B - O;
 
 f32 C = Cross(U, V);
 f32 LU = Dot(U, U);
 f32 LV = Dot(V, V);
 
 b32 Result = (C > 0.0f || (C == 0.0f && LU < LV));
 return Result;
}

internal void
AngleSort(u32 PointCount, v2 *Points)
{
 if (PointCount > 0)
 {
  v2 BottomLeftMostPoint = Points[0];
  for (u32 PointIndex = 1;
       PointIndex < PointCount;
       ++PointIndex)
  {
   v2 Point = Points[PointIndex];
   if (Point.Y < BottomLeftMostPoint.Y ||
       (Point.Y == BottomLeftMostPoint.Y && Point.X < BottomLeftMostPoint.X))
   {
    BottomLeftMostPoint = Point;
   }
  }
  
  //- Bubble sort
  b32 Sorting = true;
  for (u32 Iteration = 0;
       Sorting;
       ++Iteration)
  {
   Sorting = false;
   for (u32 J = 1; J < PointCount - Iteration; ++J)
   {
    v2 A = Points[J-1];
    v2 B = Points[J];
    if (AngleCompareLess(B, A, BottomLeftMostPoint))
    {
     Points[J-1] = B;
     Points[J] = A;
     Sorting = true;
    }
   }
  }
 }
}

internal u32
RemoveDupliatesSorted(u32 PointCount, v2 *Points)
{
 u32 UniqueCount = 0;
 if (PointCount > 0)
 {
  UniqueCount = 1;
  for (u32 PointIndex = 1;
       PointIndex < PointCount;
       ++PointIndex)
  {
   if (Points[PointIndex] != Points[UniqueCount-1])
   {
    Points[UniqueCount++] = Points[PointIndex];
   }
  }
 }
 
 return UniqueCount;
}

internal u32
CalcConvexHull(u32 PointCount, v2 *Points, v2 *OutPoints)
{
 u32 HullPointCount = 0;
 if (PointCount > 0)
 {
  MemoryCopy(OutPoints, Points, PointCount * SizeOf(OutPoints[0]));
  AngleSort(PointCount, OutPoints);
  u32 UniquePointCount = RemoveDupliatesSorted(PointCount, OutPoints);
  
  HullPointCount = 1;
  for (u32 PointIndex = 1; PointIndex < UniquePointCount; ++PointIndex)
  {
   v2 Point = OutPoints[PointIndex];
   while (HullPointCount >= 2)
   {
    v2 O = OutPoints[HullPointCount - 2];
    v2 U = OutPoints[HullPointCount - 1] - O;
    v2 V = Point - O;
    
    if (Cross(U, V) <= 0.0f)
    {
     --HullPointCount;
    }
    else
    {
     break;
    }
   }
   
   OutPoints[HullPointCount++] = Point;
  }
 }
 
 return HullPointCount;
}

// NOTE(hbr): Trinale-strip-based, no mitter, no-spiky line version.
// TODO(hbr): Loop logic is very ugly but works. Clean it up.
// Might have only work because we need only loop on convex hull order points.
internal vertex_array
ComputeVerticesOfThickLine(arena *Arena, u32 PointCount, v2 *LinePoints, f32 Width, b32 Loop)
{
 u32 N = PointCount;
 if (Loop) N += 2;
 
 u32 MaxVertexCount = 0;
 if (N >= 2) MaxVertexCount = 2*2 + 4 * N;
 
 v2 *Vertices = PushArrayNonZero(Arena, MaxVertexCount, v2);
 
 u32 VertexIndex = 0;
 b32 IsLastInside = false;
 
 if (!Loop && PointCount >= 2)
 {
  v2 A = LinePoints[0];
  v2 B = LinePoints[1];
  
  v2 V_Line = B - A;
  rotation2d NV_Line = Rotate90DegreesAntiClockwise(Rotation2D(V_Line));
  Normalize(&NV_Line.V);
  
  Vertices[VertexIndex + 0] = (A + 0.5f * Width * NV_Line.V);
  Vertices[VertexIndex + 1] = (A - 0.5f * Width * NV_Line.V);
  
  VertexIndex += 2;
  
  IsLastInside = false;
 }
 
 for (u32 PointIndex = 1;
      PointIndex + 1 < N;
      ++PointIndex)
 {
  v2 A = LinePoints[PointIndex - 1];
  v2 B = LinePoints[(PointIndex + 0) % PointCount];
  v2 C = LinePoints[(PointIndex + 1) % PointCount];
  
  v2 V_Line = B - A;
  Normalize(&V_Line);
  rotation2d NV_Line = Rotate90DegreesAntiClockwise(Rotation2D(V_Line));
  
  v2 V_Succ = C - B;
  Normalize(&V_Succ);
  rotation2d NV_Succ = Rotate90DegreesAntiClockwise(Rotation2D(V_Succ));
  
  b32 LeftTurn = (Cross(V_Line, V_Succ) >= 0.0f);
  f32 TurnedHalfWidth = (LeftTurn ? 1.0f : -1.0f) * 0.5f * Width;
  
  line_intersection Intersection = LineIntersection(A + TurnedHalfWidth * NV_Line.V,
                                                    B + TurnedHalfWidth * NV_Line.V,
                                                    B + TurnedHalfWidth * NV_Succ.V,
                                                    C + TurnedHalfWidth * NV_Succ.V);
  
  v2 IntersectionPoint = {};
  if (Intersection.IsOneIntersection)
  {
   IntersectionPoint = Intersection.IntersectionPoint;
  }
  else
  {
   IntersectionPoint = B + TurnedHalfWidth * NV_Line.V;
  }
  
  v2 B_Line = B - TurnedHalfWidth * NV_Line.V;
  v2 B_Succ = B - TurnedHalfWidth * NV_Succ.V;
  
  if ((LeftTurn && IsLastInside) || (!LeftTurn && !IsLastInside))
  {
   Vertices[VertexIndex + 0] = B_Line;
   Vertices[VertexIndex + 1] = IntersectionPoint;
   Vertices[VertexIndex + 2] = B_Succ;
   
   VertexIndex += 3;
  }
  else
  {
   Vertices[VertexIndex + 0] = IntersectionPoint;
   Vertices[VertexIndex + 1] = B_Line;
   Vertices[VertexIndex + 2] = IntersectionPoint;
   Vertices[VertexIndex + 3] = B_Succ;
   
   VertexIndex += 4;
  }
  
  IsLastInside = !LeftTurn;
 }
 
 if (!Loop)
 {
  if (PointCount >= 2)
  {
   v2 A = LinePoints[N-2];
   v2 B = LinePoints[N-1];
   
   v2 V_Line = B - A;
   rotation2d NV_Line = Rotate90DegreesAntiClockwise(Rotation2D(V_Line));
   Normalize(&NV_Line.V);
   
   v2 B_Inside  = B + 0.5f * Width * NV_Line.V;
   v2 B_Outside = B - 0.5f * Width * NV_Line.V;
   
   if (IsLastInside)
   {
    Vertices[VertexIndex + 0] = B_Outside;
    Vertices[VertexIndex + 1] = B_Inside;
   }
   else
   {
    Vertices[VertexIndex + 0] = B_Inside;
    Vertices[VertexIndex + 1] = B_Outside;
   }
   
   VertexIndex += 2;
   
   IsLastInside = false;
  }
 }
 else if (N >= 2)
 {
  if (IsLastInside)
  {
   Vertices[VertexIndex + 0] = Vertices[1];
   Vertices[VertexIndex + 1] = Vertices[0];
   
   VertexIndex += 2;
   
   IsLastInside = true;
  }
  else
  {
   Vertices[VertexIndex + 0] = Vertices[0];
   Vertices[VertexIndex + 1] = Vertices[1];
   
   VertexIndex += 2;
   
   IsLastInside = false;
  }
 }
 
 vertex_array Result = {};
 Result.VertexCount = VertexIndex;
 Result.Vertices = Vertices;
 Result.Primitive = Primitive_TriangleStrip;
 
 return Result;
}

internal void
EquidistantPoints(f32 *Ti, u32 N, f32 A, f32 B)
{
 if (N > 1)
 {
  f32 Delta_T = (B-A) / (N-1);
  f32 T = A;
  for (u32 I = 0; I < N; ++I)
  {
   Ti[I] = T;
   T += Delta_T;
  }
 }
}

internal void
ChebyshevPoints(f32 *Ti, u32 N)
{
 for (u32 K = 0; K < N; ++K)
 {
  Ti[K] = CosF32(PiF32 * (2*K+1) / (2*N));
 }
}

internal void
BarycentricOmega(f32 *Omega, f32 *Ti, u32 N)
{
 for (u32 I = 0; I < N; ++I)
 {
  f32 W = 1.0f;
  for (u32 J = 0; J < N; ++J)
  {
   if (J != I)
   {
    W *= (Ti[I] - Ti[J]);
   }
  }
  Omega[I] = 1.0f / W;
 }
}

// TODO(hbr): Figure out with it doesn't work
internal void
BarycentricOmegaWerner(f32 *Omega, f32 *Ti, u32 N)
{
 temp_arena Temp = TempArena(0);
 
 f32 *Mem = PushArrayNonZero(Temp.Arena, N*N, f32);
 
 Mem[Index2D(0, 0, N)] = 1.0f;
 for (u32 K = 1; K < N; ++K)
 {
  Mem[Index2D(0, K, N)] = 0.0f;
 }
 
 for (u32 I = 1; I < N; ++I)
 {
  for (u32 K = 0; K <= N-I; ++K)
  {
   Mem[Index2D(I, K, N)] = Mem[Index2D(I-1, K, N)] / (Ti[K] - Ti[I]);
   Mem[Index2D(K+1, I, N)] = Mem[Index2D(K, I, N)] - Mem[Index2D(I, K, N)];
  }
 }
 
 for (u32 I = 0; I < N; ++I)
 {
  Omega[I] = Mem[Index2D(N-1, I, N)];
 }
 
 EndTemp(Temp);
}

internal void
BarycentricOmegaEquidistant(f32 *Omega, f32 *Ti, u32 N)
{
 if (N <= 1)
 {
  // NOTE(hbr): Do nothing
 }
 else
 {
  f32 W0 = 1.0f;
  for (u32 J = 1; J < N; ++J)
  {
   W0 /= (Ti[0] - Ti[J]);
  }
  
  Omega[0] = W0;
  
  f32 W = W0;
  for (u32 I = 0; I < N-1; ++I)
  {
   i32 Num = I - N;
   f32 Frac = Cast(f32)Num / (I + 1);
   W *= Frac;
   Omega[I+1] = W;
  }
 }
}

inline internal f32
PowerOf2(u32 K)
{
 f32 Result = 1.0f;
 f32 A = 2.0f;
 while (K)
 {
  if (K & 1) Result = Result*A;
  A = A*A;
  K >>= 1;
 }
 
 return Result;
}

internal void
BarycentricOmegaChebychev(f32 *Omega, u32 N)
{
 if (N > 0)
 {
  f32 PowOf2 = PowerOf2(N-1);
  for (u32 I = 0; I < N; ++I)
  {
   f32 Num = PowOf2 * SinF32(PiF32 * (2*I+1) / (2*N));
   f32 Den = (N * SinF32(PiF32 * (2*I+1) * 0.5f));
   Omega[I] = Num / Den;
  }
 }
}

internal f32
BarycentricEvaluate(f32 T, f32 *Omega, f32 *Ti, f32 *Y, u32 N)
{
 for (u32 I = 0; I < N; ++I)
 {
  if (T == Ti[I])
  {
   return Y[I];
  }
 }
 
 f32 Num = 0.0f;
 f32 Den = 0.0f;
 for (u32 I = 0; I < N; ++I)
 {
  f32 Fraction = Omega[I] / (T - Ti[I]);
  Num += Y[I] * Fraction;
  Den += Fraction;
 }
 
 f32 Result = Num / Den;
 return Result;
}

// NOTE(hbr): Compute directly from definition
internal void
NewtonBeta(f32 *Beta, f32 *Ti, f32 *Y, u32 N)
{
 temp_arena Temp = TempArena(0);
 
 f32 *Mem = PushArrayNonZero(Temp.Arena, N*N, f32);
 for (u32 J = 0; J < N; ++J)
 {
  Mem[J] = Y[J];
 }
 
 for (u32 I = 1; I < N; ++I)
 {
  for (u32 J = 0; J < N-I; ++J)
  {
   Mem[I*N + J] = (Mem[(I-1)*N + J+1] - Mem[(I-1)*N + J]) / (Ti[J+I] - Ti[J]);
  }
 }
 
 for (u32 I = 0; I < N; ++I)
 {
  Beta[I] = Mem[I*N + 0];
 }
 
 EndTemp(Temp);
}

// NOTE(hbr): Memory optimized version
internal void
NewtonBetaFast(f32 *Beta, f32 *Ti, f32 *Y, u32 N)
{
 for (u32 J = 0; J < N; ++J)
 {
  Beta[J] = Y[J];
 }
 
 for (u32 I = 1; I < N; ++I)
 {
  f32 Save = Beta[I-1];
  for (u32 J = 0; J < N-I; ++J)
  {
   f32 Temp = Beta[I+J];
   Beta[I+J] = (Beta[I+J] - Save) / (Ti[I+J] - Ti[J]);
   Save = Temp;
  }
 }
}

internal f32
NewtonEvaluate(f32 T, f32 *Beta, f32 *Ti, u32 N)
{
 f32 Result = 0.0f;
 for (u32 I = N; I > 0; --I)
 {
  Result = Result * (T - Ti[I-1]) + Beta[I-1];
 }
 
 return Result;
}

internal f32
NewtonEvaluateUnrolled4x(f32 T, f32 *Beta, f32 *Ti, u32 N)
{
 f32 Result = 0.0f;
 
 u32 I = N;
 while (I >= 4)
 {
  Result = fmaf(Result, (T - Ti[I-1]), Beta[I-1]);
  Result = fmaf(Result, (T - Ti[I-2]), Beta[I-2]);
  Result = fmaf(Result, (T - Ti[I-3]), Beta[I-3]);
  Result = fmaf(Result, (T - Ti[I-4]), Beta[I-4]);
  I -= 4;
 }
 
 while (I >= 1)
 {
  Result = fmaf(Result, (T - Ti[I-1]), Beta[I-1]);
  --I;
 }
 
 return Result;
}

internal i32
GaussianElimination(f32 *A, f32 *B, u32 Rows, u32 Cols)
{
 i32 Result = 0;
 temp_arena Temp = TempArena(0);
 
 u32 *Pos = PushArray(Temp.Arena, Cols, u32);
 f32 Det = 1;
 u32 Rank = 0;
 for (u32 Col = 0, Row = 0; Col < Cols && Row < Rows; ++Col)
 {
  u32 MaxRow = Row;
  f32 MaxRowValue = Abs(A[Index2D(MaxRow, Col, Cols)]);
  for (u32 I = Row; I < Rows; I++)
  {
   f32 CandValue = Abs(A[Index2D(I, Col, Cols)]);
   if (CandValue > MaxRowValue)
   {
    MaxRow = I;
    MaxRowValue = CandValue;
   }
  }
  if (Abs(A[Index2D(MaxRow, Col, Cols)]) < F32_EPS)
  {
   Det = 0;
   continue;
  }
  
  for (u32 I = Col; I < Cols; I++)
  {
   Swap(A[Index2D(Row, I, Cols)],
        A[Index2D(MaxRow, I, Cols)],
        f32);
  }
  Swap(B[Row], B[MaxRow], f32);
  
  if (Row != MaxRow)
  {
   Det = -Det;
  }
  Det *= A[Index2D(Row, Col, Cols)];
  
  Pos[Col] = Row+1;
  
  for (u32 I = 0; I < Rows; I++)
  {
   if (I != Row && Abs(A[Index2D(I, Col, Cols)]) > F32_EPS)
   {
    f32 C = A[Index2D(I, Col, Cols)] / A[Index2D(Row, Col, Cols)];
    for (u32 J = Col; J < Cols; J++)
    {
     A[Index2D(I, J, Cols)] -= C * A[Index2D(Row, J, Cols)];
    }
    B[I] -= C * B[Row];
   }
  }
  ++Row; ++Rank;
 }
 
 for (u32 I = 0; I < Cols; I++)
 {
  f32 Solution = 0.0f;
  u32 Index = Pos[I];
  if(Index != 0)
  {
   Solution = B[Index-1] / A[Index2D(Index-1, I, Cols)];
  }
  B[I] = Solution;
 }
 
 for (u32 I = 0; I < Rows; I++)
 {
  f32 Sum = 0;
  for (u32 J = 0; J < Cols; J++)
  {
   Sum += B[J] * A[Index2D(I, J, Cols)];
  }
  if (Abs(Sum - B[I]) > F32_EPS)
  {
   Result = -1;
   break;
  }
 }
 
 if (Result != -1)
 {
  u32 FreeVars = 0;
  for (u32 I = 0; I < Cols; I++)
  {
   if (Pos[I] != 0)
   {
    ++FreeVars;
   }
  }
  Result = FreeVars;
 }
 
 EndTemp(Temp);
 
 return Result;
}

internal void
BSplineBaseKnots(b_spline_knot_params KnotParams, f32 *Knots)
{
 EquidistantPoints(Knots + KnotParams.Degree,
                   KnotParams.PartitionSize,
                   KnotParams.A,
                   KnotParams.B);
}

internal void
BSplineKnotsNaturalExtension(b_spline_knot_params KnotParams, f32 *Knots)
{
 for (u32 I = 0; I < KnotParams.Degree; ++I)
 {
  Knots[I] = KnotParams.A;
 }
 for (u32 I = 0; I < KnotParams.Degree; ++I)
 {
  Knots[KnotParams.Degree + KnotParams.PartitionSize + I] = KnotParams.B;
 }
}

internal void
BSplineKnotsPeriodicExtension(b_spline_knot_params KnotParams, f32 *Knots)
{
 f32 A = KnotParams.A;
 f32 B = KnotParams.B;
 f32 *t = Knots + KnotParams.Degree;
 i32 n = KnotParams.PartitionSize - 1;
 i32 m = KnotParams.Degree;
 
 for (i32 i = 1; i <= m; ++i)
 {
  t[-i] = A - B + t[n - i];
  t[n + i] = B - A + t[i];
 }
}

// TODO(hbr): This can be ppb optimized to use only O(m) memory
internal v2
BSplineEvaluate(f32 T, v2 *Controls, b_spline_knot_params KnotParams, f32 *Knots)
{
 temp_arena Temp = TempArena(0);
 
 // NOTE(hbr): This is unfortunately necessary. Otherwise it freaks out on tail.
 T = Clamp(T, KnotParams.A, KnotParams.B - F32_EPS);
 //T = Clamp(T, Knots[0] + 10*F32_EPS, Knots[KnotParams.KnotCount - 1] - 10*F32_EPS);
 //T = Clamp(T, KnotParams.A, KnotParams.B);
 
 i32 Degree = Cast(i32)KnotParams.Degree;
 i32 PartitionSize = Cast(i32)KnotParams.PartitionSize;
 
 i32 Rows = Cast(i32)Degree;
 i32 Cols = Cast(i32)Degree;
 // TODO(hbr): I noticed, that it is a bug to not add 1 here, investiagate whether this is enough or there is something more to it.
 v2 *C = PushArray2DNonZero(Temp.Arena, Rows+1, Cols+1, v2);
 
#define c(k, i) C[Index2D(k, i+m, Cols)]
 
 Controls += Degree;
 
 i32 m = Degree;
 i32 n = PartitionSize - 1;
 
 f32 *t = Knots + KnotParams.Degree;
 
 i32 j = -m;
 for (; j+1 < n+m; ++j)
 {
  if (t[j] <= T && T < t[j + 1])
  {
   break;
  }
 }
 
 i32 start_i = j-m;
 for (i32 i = start_i; i <= j; ++i)
 {
  c(0, i-start_i) = Controls[i];
 }
 
 for (i32 k = 1; k <= m; ++k)
 {
  i32 start_i = j-m+k;
  for (i32 i = start_i; i <= j; ++i)
  {
   v2 Num = (T - t[i]) * c(k-1, i-(start_i-1)) + (t[m+i+1-k] - T) * c(k-1, i-1-(start_i-1));
   f32 Den = t[m+i+1-k] - t[i];
   c(k, i-start_i) = Num / Den;
  }
 }
 
 v2 Result = c(m, j-start_i);
 
 EndTemp(Temp);
 
#undef c
 
 return Result;
}

// TODO(hbr): It doesn't work quite well for Degree == 1.
internal v2
NURBS_EvaluateScalar(f32 T,
                     v2 *Controls,
                     f32 *Weights,
                     b_spline_knot_params KnotParams,
                     f32 *Knots)
{
 temp_arena Temp = TempArena(0);
 
 // NOTE(hbr): This is unfortunately necessary. Otherwise it freaks out on tail.
 T = Clamp(T, KnotParams.A, KnotParams.B - F32_EPS);
 //T = Clamp(T, Knots[0] + 10*F32_EPS, Knots[KnotParams.KnotCount - 1] - 10*F32_EPS);
 //T = Clamp(T, KnotParams.A, KnotParams.B);
 
 i32 Degree = Cast(i32)KnotParams.Degree;
 i32 PartitionSize = Cast(i32)KnotParams.PartitionSize;
 
 i32 Rows = Cast(i32)Degree;
 i32 Cols = Cast(i32)Degree;
 // TODO(hbr): I noticed, that it is a bug to not add 1 here, investiagate whether this is enough or there is something more to it.
 v2 *C = PushArray2DNonZero(Temp.Arena, Rows+1, Cols+1, v2);
 f32 *W = PushArray2DNonZero(Temp.Arena, Rows+1, Cols+1, f32);
 
#define c(k, i) C[Index2D(k, i+m, Cols)]
#define w(k, i) W[Index2D(k, i+m, Cols)]
 
 Controls += Degree;
 Weights += Degree;
 
 i32 m = Degree;
 i32 n = PartitionSize - 1;
 
 f32 *t = Knots + KnotParams.Degree;
 
 i32 j = -m;
 for (; j+1 < n+m; ++j)
 {
  if (t[j] <= T && T < t[j + 1])
  {
   break;
  }
 }
 
 i32 start_i = j-m;
 for (i32 i = start_i; i <= j; ++i)
 {
  c(0, i-start_i) = Controls[i];
  w(0, i-start_i) = Weights[i];
 }
 
 for (i32 k = 1; k <= m; ++k)
 {
  i32 start_i = j-m+k;
  for (i32 i = start_i; i <= j; ++i)
  {
   f32 alpha = (T - t[i]) / (t[m+i+1-k] - t[i]);
   f32 new_w = alpha * w(k-1, i-(start_i-1)) + (1-alpha) * w(k-1, i-1-(start_i-1));
   w(k, i-start_i) = new_w;
   c(k, i-start_i) = alpha * w(k-1, i-(start_i-1))/new_w * c(k-1, i-(start_i-1)) + (1-alpha) * w(k-1, i-1-(start_i-1))/new_w * c(k-1, i-1-(start_i-1));
  }
 }
 
 v2 Result = c(m, j-start_i);
 
 EndTemp(Temp);
 
#undef c
#undef w
 
 return Result;
}

#include <stdlib.h>
#include <stdio.h>
#include <xmmintrin.h>  // SSE
#include <emmintrin.h>  // SSE2 (if needed)

internal int
find_span_scalar(u32 n, u32 m, f32 *Knots, f32 t) {
 u32 lo = m;
 u32 hi = n + m - 1;
 if (t >= Knots[hi + 1]) return hi;
 while (lo <= hi) {
  u32 mid = (lo + hi) >> 1;
  if (t < Knots[mid]) {
   if (mid == 0) break;
   hi = mid - 1;
  } else if (t >= Knots[mid + 1]) {
   lo = mid + 1;
  } else {
   return (int)mid;
  }
 }
 return (int)lo;
}

internal void
NURBS_EvaluateSSE(v2 *P, f32 *W, u32 n, u32 m, f32 *Knots, f32 t4[4], v2 out[4]) {
 u32 N_ctrl = n + m;
 
 // find spans per lane
 int span[4];
 for (int lane = 0; lane < 4; ++lane) {
  span[lane] = find_span_scalar(n, m, Knots, t4[lane]);
 }
 
 // allocate D_x[k], D_y[k], D_w[k] as __m128 for k=0..m
 __m128 *D_x = (__m128*)malloc((m+1)*sizeof(__m128));
 __m128 *D_y = (__m128*)malloc((m+1)*sizeof(__m128));
 __m128 *D_w = (__m128*)malloc((m+1)*sizeof(__m128));
 
 // initialize lanes: for k = 0..m, D_*[k] holds values for lanes 0..3 at that k
 for (u32 k = 0; k <= (u32)m; ++k) {
  f32 lane_x[4], lane_y[4], lane_w[4];
  for (int lane = 0; lane < 4; ++lane) {
   int j = span[lane];
   u32 idx = (u32)(j - m + k);
   // bounds checking (safety): clamp idx
   if (idx >= N_ctrl) {
    lane_x[lane] = 0.0f; lane_y[lane] = 0.0f; lane_w[lane] = 0.0f;
   } else {
    lane_x[lane] = P[idx].X * W[idx];
    lane_y[lane] = P[idx].Y * W[idx];
    lane_w[lane] = W[idx];
   }
  }
  // Note: _mm_set_ps takes values in reverse lane order: (w3, w2, w1, w0)
  D_x[k] = _mm_set_ps(lane_x[3], lane_x[2], lane_x[1], lane_x[0]);
  D_y[k] = _mm_set_ps(lane_y[3], lane_y[2], lane_y[1], lane_y[0]);
  D_w[k] = _mm_set_ps(lane_w[3], lane_w[2], lane_w[1], lane_w[0]);
 }
 
 // pack t into vector
 __m128 T = _mm_set_ps(t4[3], t4[2], t4[1], t4[0]);
 
 // de Boor: for r=1..m, for k=m..r
 for (u32 r = 1; r <= (u32)m; ++r) {
  for (int k = (int)m; k >= (int)r; --k) {
   // compute per-lane alpha values and pack into vector
   f32 alpha_lane[4];
   for (int lane = 0; lane < 4; ++lane) {
    int j = span[lane];
    u32 i = (u32)(j - m + k);
    f32 num = t4[lane] - Knots[i];
    f32 den = Knots[i + m + 1 - r] - Knots[i];
    alpha_lane[lane] = (den == 0.0f) ? 0.0f : (num / den);
   }
   __m128 alpha = _mm_set_ps(alpha_lane[3], alpha_lane[2], alpha_lane[1], alpha_lane[0]);
   __m128 one = _mm_set1_ps(1.0f);
   __m128 one_minus_alpha = _mm_sub_ps(one, alpha);
   
   // D[k] = (1-alpha)*D[k-1] + alpha*D[k]  (vectorized)
   __m128 dx_k   = D_x[k];
   __m128 dx_km1 = D_x[k-1];
   __m128 dy_k   = D_y[k];
   __m128 dy_km1 = D_y[k-1];
   __m128 dw_k   = D_w[k];
   __m128 dw_km1 = D_w[k-1];
   
   __m128 nx = _mm_add_ps(_mm_mul_ps(one_minus_alpha, dx_km1), _mm_mul_ps(alpha, dx_k));
   __m128 ny = _mm_add_ps(_mm_mul_ps(one_minus_alpha, dy_km1), _mm_mul_ps(alpha, dy_k));
   __m128 nw = _mm_add_ps(_mm_mul_ps(one_minus_alpha, dw_km1), _mm_mul_ps(alpha, dw_k));
   
   D_x[k] = nx;
   D_y[k] = ny;
   D_w[k] = nw;
  }
 }
 
 // result in D_x[m], D_y[m], D_w[m]
 __m128 rx = D_x[m];
 __m128 ry = D_y[m];
 __m128 rw = D_w[m];
 
 // divide (rx/rw, ry/rw)
 // guard against zero: we perform division; if rw==0 lane becomes INF/NaN -> we clamp afterwards
 __m128 inv_rw = _mm_div_ps(_mm_set1_ps(1.0f), rw);
 __m128 outx = _mm_mul_ps(rx, inv_rw);
 __m128 outy = _mm_mul_ps(ry, inv_rw);
 
 // store results back to out[4]
 f32 out_x[4], out_y[4];
 _mm_storeu_ps(out_x, outx); // stores lane order (w0..w3) as out_x[0..3]
 _mm_storeu_ps(out_y, outy);
 
 for (int lane = 0; lane < 4; ++lane) {
  out[lane].X = out_x[lane];
  out[lane].Y = out_y[lane];
 }
 
 free(D_x);
 free(D_y);
 free(D_w);
}

internal void
NURBS_EvaluateAVX2(v2 *P, f32 *W, u32 n, u32 m, f32 *Knots, f32 t8[8], v2 out[8])
{
 u32 N_ctrl = n + m;
 
 // find spans per lane (scalar)
 int span[8];
 for (int lane = 0; lane < 8; ++lane) {
  span[lane] = find_span_scalar(n, m, Knots, t8[lane]);
 }
 
 // allocate D_x[k], D_y[k], D_w[k] as __m256 for k=0..m
 __m256 *D_x = (__m256*)malloc((size_t)(m + 1) * sizeof(__m256));
 __m256 *D_y = (__m256*)malloc((size_t)(m + 1) * sizeof(__m256));
 __m256 *D_w = (__m256*)malloc((size_t)(m + 1) * sizeof(__m256));
 if (!D_x || !D_y || !D_w) {
  // allocation failed: write zeros to output and return
  for (int lane = 0; lane < 8; ++lane) { out[lane].X = 0.0f; out[lane].Y = 0.0f; }
  free(D_x); free(D_y); free(D_w);
  return;
 }
 
 // initialize lanes: for k = 0..m, D_*[k] holds values for lanes 0..7 at that k
 for (u32 k = 0; k <= m; ++k) {
  f32 lane_x[8], lane_y[8], lane_w[8];
  for (int lane = 0; lane < 8; ++lane) {
   int j = span[lane];
   u32 idx = (u32)( (int)j - (int)m + (int)k ); // careful with signed arithmetic
   if (idx >= N_ctrl) {
    lane_x[lane] = 0.0f; lane_y[lane] = 0.0f; lane_w[lane] = 0.0f;
   } else {
    lane_x[lane] = P[idx].X * W[idx];
    lane_y[lane] = P[idx].Y * W[idx];
    lane_w[lane] = W[idx];
   }
  }
  // _mm256_set_ps takes arguments in order (w7,...,w0)
  D_x[k] = _mm256_set_ps(lane_x[7], lane_x[6], lane_x[5], lane_x[4],
                         lane_x[3], lane_x[2], lane_x[1], lane_x[0]);
  D_y[k] = _mm256_set_ps(lane_y[7], lane_y[6], lane_y[5], lane_y[4],
                         lane_y[3], lane_y[2], lane_y[1], lane_y[0]);
  D_w[k] = _mm256_set_ps(lane_w[7], lane_w[6], lane_w[5], lane_w[4],
                         lane_w[3], lane_w[2], lane_w[1], lane_w[0]);
 }
 
 // pack t into vector (t8[0] -> lane 0, etc.)
 __m256 T = _mm256_set_ps(t8[7], t8[6], t8[5], t8[4], t8[3], t8[2], t8[1], t8[0]);
 
 __m256 one = _mm256_set1_ps(1.0f);
 
 // de Boor: for r=1..m, for k=m..r
 for (u32 r = 1; r <= m; ++r) {
  for (int k = (int)m; k >= (int)r; --k) {
   // compute per-lane alpha values and pack into vector
   f32 alpha_lane[8];
   for (int lane = 0; lane < 8; ++lane) {
    int j = span[lane];
    u32 i = (u32)((int)j - (int)m + k);
    // bounds-safety: if i or i + m + 1 - r out of range, treat den as zero
    f32 num = t8[lane] - Knots[i];
    f32 den = Knots[i + m + 1 - r] - Knots[i];
    alpha_lane[lane] = (den == 0.0f) ? 0.0f : (num / den);
   }
   __m256 alpha = _mm256_set_ps(
                                alpha_lane[7], alpha_lane[6], alpha_lane[5], alpha_lane[4],
                                alpha_lane[3], alpha_lane[2], alpha_lane[1], alpha_lane[0]
                                );
   __m256 one_minus_alpha = _mm256_sub_ps(one, alpha);
   
   // vectorized de Boor step: D[k] = (1-alpha)*D[k-1] + alpha*D[k]
   __m256 dx_k   = D_x[k];
   __m256 dx_km1 = D_x[k-1];
   __m256 dy_k   = D_y[k];
   __m256 dy_km1 = D_y[k-1];
   __m256 dw_k   = D_w[k];
   __m256 dw_km1 = D_w[k-1];
   
   __m256 nx = _mm256_add_ps(_mm256_mul_ps(one_minus_alpha, dx_km1),
                             _mm256_mul_ps(alpha, dx_k));
   __m256 ny = _mm256_add_ps(_mm256_mul_ps(one_minus_alpha, dy_km1),
                             _mm256_mul_ps(alpha, dy_k));
   __m256 nw = _mm256_add_ps(_mm256_mul_ps(one_minus_alpha, dw_km1),
                             _mm256_mul_ps(alpha, dw_k));
   
   D_x[k] = nx;
   D_y[k] = ny;
   D_w[k] = nw;
  }
 }
 
 // result in D_x[m], D_y[m], D_w[m]
 __m256 rx = D_x[m];
 __m256 ry = D_y[m];
 __m256 rw = D_w[m];
 
 // divide: out = (rx/rw, ry/rw)
 // compute reciprocal of rw (may produce INF if zero; caller can clamp)
 __m256 inv_rw = _mm256_div_ps(_mm256_set1_ps(1.0f), rw);
 __m256 outx = _mm256_mul_ps(rx, inv_rw);
 __m256 outy = _mm256_mul_ps(ry, inv_rw);
 
 // store results back to out[8]
 f32 out_x[8], out_y[8];
 _mm256_storeu_ps(out_x, outx); // stores lanes as out_x[0..7]
 _mm256_storeu_ps(out_y, outy);
 
 for (int lane = 0; lane < 8; ++lane) {
  out[lane].X = out_x[lane];
  out[lane].Y = out_y[lane];
 }
 
 free(D_x);
 free(D_y);
 free(D_w);
}

// NOTE(hbr): Those should be local conveniance internal, but impossible in C.
inline internal f32 Hi(f32 *Ti, u32 I) { return Ti[I+1] - Ti[I]; }
inline internal f32 Bi(f32 *Ti, f32 *Y, u32 I) { return 1.0f / Hi(Ti, I) * (Y[I+1] - Y[I]); }
inline internal f32 Vi(f32 *Ti, u32 I) { return 2.0f * (Hi(Ti, I) + Hi(Ti, I+1)); }
inline internal f32 Ui(f32 *Ti, f32 *Y, u32 I) { return 6.0f * (Bi(Ti, Y, I+1) - Bi(Ti, Y, I)); }
internal void
CubicSplineNaturalM(f32 *M, f32 *Ti, f32 *Y, u32 N)
{
 if (N == 0) {} // NOTE(hbr): Nothing to calculate
 else if (N == 1) { M[0] = 0.0f; }
 else if (N == 2) { M[0] = M[N-1] = 0.0f; }
 else
 {
  temp_arena Temp = TempArena(0);
  
  M[0] = M[N-1] = 0.0f;
  
  f32 *Diag = PushArrayNonZero(Temp.Arena, N, f32);
  for (u32 I = 1; I < N-1; ++I)
  {
   Diag[I] = Vi(Ti, I-1);
   M[I] = Ui(Ti, Y, I-1);
  }
  
  for (u32 I = 2; I < N-1; ++I)
  {
   // NOTE(hbr): Maybe optimize Hi
   f32 Multiplier = Hi(Ti, I-1) / Diag[I-1];
   Diag[I] -= Multiplier * Hi(Ti, I-1);
   M[I] -= Multiplier * M[I-1];
  }
  
  for (u32 I = N-2; I > 1; --I)
  {
   f32 Multiplier = Hi(Ti, I-1) / Diag[I];
   M[I-1] -= Multiplier * M[I];
   M[I] /= Diag[I];
  }
  M[1] /= Diag[1];
  
  EndTemp(Temp);
 }
}

internal b_spline_degree_bounds
BSplineDegreeBounds(u32 ControlPointCount)
{
 b_spline_degree_bounds Result = {};
 if (ControlPointCount > 1)
 {
  Result.MinDegree = 1;
  Result.MaxDegree = ControlPointCount - 1;
 }
 return Result;
}

internal b_spline_knot_params
BSplineKnotParamsFromDegree(u32 Degree, u32 ControlPointCount)
{
 b_spline_degree_bounds DegreeBounds = BSplineDegreeBounds(ControlPointCount);
 Degree = Clamp(Degree, DegreeBounds.MinDegree, DegreeBounds.MaxDegree);
 
 u32 PartitionSize = ControlPointCount - Degree + 1;
 if (ControlPointCount == 0)
 {
  PartitionSize = 0;
 }
 Assert(Cast(i32)PartitionSize >= 0);
 
 u32 KnotCount = 2*Degree + PartitionSize;
 
 b_spline_knot_params Result = {};
 Result.Degree = Degree;
 Result.PartitionSize = PartitionSize;
 Result.KnotCount = KnotCount;
 Result.A = 0.0f;
 Result.B = 1.0f;
 
 return Result;
}

// NOTE(hbr): Those should be local conveniance internal, but impossible in C.
inline internal f32 Hi(f32 *Ti, u32 I, u32 N) { if (I == N-1) I = 0; return Ti[I+1] - Ti[I]; }
inline internal f32 Yi(f32 *Y, u32 I, u32 N) { if (I == N) I = 1; return Y[I]; }
inline internal f32 Bi(f32 *Ti, f32 *Y, u32 I, u32 N) { return 1.0f / Hi(Ti, I, N) * (Yi(Y, I+1, N) - Yi(Y, I, N)); }
inline internal f32 Vi(f32 *Ti, u32 I, u32 N) { return 2.0f * (Hi(Ti, I, N) + Hi(Ti, I+1, N)); }
inline internal f32 Ui(f32 *Ti, f32 *Y, u32 I, u32 N) { return 6.0f * (Bi(Ti, Y, I+1, N) - Bi(Ti, Y, I, N)); }
internal void
CubicSplinePeriodicM(f32 *M, f32 *Ti, f32 *Y, u32 N)
{
 if (N == 0) {} // NOTE(hbr): Nothing to calculate
 else if (N == 1) { M[0] = 0.0f; }
 else if (N == 2) { M[0] = M[N-1] = 0.0f; }
 else
 {
  Assert(Y[0] == Y[N-1]);
  temp_arena Temp = TempArena(0);
  
  f32 *A = PushArray(Temp.Arena, (N-1)*(N-1), f32);
  for (u32 I = 0; I < N-1; ++I)
  {
   A[Index2D(I, I, N-1)] = Vi(Ti, I, N);
  }
  for (u32 I = 1; I < N-1; ++I)
  {
   A[Index2D(I-1, I, N-1)] = Hi(Ti, I, N);
  }
  for (u32 I = 1; I < N-1; ++I)
  {
   A[Index2D(I, I-1, N-1)] = Hi(Ti, I, N);
  }
  A[Index2D(0, N-2, N-1)] = Hi(Ti, 0, N);
  A[Index2D(N-2, 0, N-1)] = Hi(Ti, 0, N);
  
  for (u32 I = 0; I < N-1; ++I)
  {
   M[I+1] = Ui(Ti, Y, I, N);
  }
  
  GaussianElimination(A, M+1, N-1, N-1);
  M[0] = M[N-1];
  
  EndTemp(Temp);
 }
}

internal f32
CubicSplineEvaluateScalar(f32 T, f32 *M, f32 *Ti, f32 *Y, u32 N)
{
 f32 Result = 0.0f;
 
 if (N == 0) {} // NOTE(hbr): Nothing to do
 else if (N == 1) { Result = Y[0]; } // NOTE(hbr): No interpolation in one point case
 else
 {
  // NOTE(hbr): Just linearly search
  u32 I = 0;
  for (I = N-2; I > 0; --I)
  {
   if (T >= Ti[I]) break;
  }
  
  // NOTE(hbr): Maybe optimize Hi here and T-Ti
  f32 Term1 = M[I+1] / (6.0f * Hi(Ti, I)) * Cube(T - Ti[I]);
  f32 Term2 = M[I] / (6.0f * Hi(Ti, I)) * Cube(Ti[I+1] - T);
  f32 Term3 = (Y[I+1] / Hi(Ti, I) - M[I+1] / 6.0f * Hi(Ti, I)) * (T - Ti[I]);
  f32 Term4 = (Y[I] / Hi(Ti, I) - Hi(Ti, I) / 6.0f * M[I]) * (Ti[I+1] - T);
  Result = Term1 + Term2 + Term3 + Term4;
 }
 
 return Result;
}

internal f32
CubicSplineEvaluateScalarWithBinarySearch(f32 T, f32 *M, f32 *Ti, f32 *Y, u32 N)
{
 f32 Result = 0.0f;
 
 if (N == 0) {} // NOTE(hbr): Nothing to do
 else if (N == 1) { Result = Y[0]; } // NOTE(hbr): No interpolation in one point case
 else
 {
  u32 I2 = 0;
  for (I2 = N-2; I2 > 0; --I2)
  {
   if (T >= Ti[I2]) break;
  }
  
  Assert(T >= Ti[0]);
  u32 Left = 1, Right = N;
  while (Left + 1 < Right)
  {
   u32 Mid = (Left + Right) >> 1;
   if (T >= Ti[Mid-1]) Left = Mid;
   else Right = Mid;
  }
  Assert(Left > 0);
  u32 I = Left-1;
  
  Assert(I == I2);
  
  // NOTE(hbr): Maybe optimize Hi here and T-Ti
  f32 Term1 = M[I+1] / (6.0f * Hi(Ti, I)) * Cube(T - Ti[I]);
  f32 Term2 = M[I] / (6.0f * Hi(Ti, I)) * Cube(Ti[I+1] - T);
  f32 Term3 = (Y[I+1] / Hi(Ti, I) - M[I+1] / 6.0f * Hi(Ti, I)) * (T - Ti[I]);
  f32 Term4 = (Y[I] / Hi(Ti, I) - Hi(Ti, I) / 6.0f * M[I]) * (Ti[I+1] - T);
  Result = Term1 + Term2 + Term3 + Term4;
 }
 
 return Result;
}

internal f32
CubicSplineEvaluateScalarWithConstantSearch(f32 T, f32 *M, f32 *Ti, f32 *Y, u32 N, cubic_spline_evaluate_iterator *It)
{
 f32 R = 0.0f;
 
 if (N == 0) {} // NOTE(hbr): Nothing to do
 else if (N == 1) { R = Y[0]; } // NOTE(hbr): No interpolation in one point case
 else
 {
  u32 I = It->I;
  if (I < N-1 && T >= Ti[I]) ++I;
  Assert(I > 0);
  Assert(T >= Ti[I-1]);
  Assert(T < Ti[I] || I == N-1);
  
  f32 One_Over_6 = 1.0f / 6.0f;
  f32 h_I = Ti[I] - Ti[I-1];
  
  f32 A = One_Over_6 * M[I-1] * Cube(Ti[I] - T);
  f32 B = One_Over_6 * M[I] * Cube(T - Ti[I-1]);
  f32 C = (Y[I-1] - One_Over_6 * M[I-1] * Sqr(h_I)) * (Ti[I] - T);
  f32 D = (Y[I] - One_Over_6 * M[I] * Sqr(h_I)) * (T - Ti[I-1]);
  R = (A + B + C + D) / h_I;
  
  It->I = I;
 }
 
 return R;
}

// NOTE(hbr): O(n) time, O(1) memory versions
internal v2
BezierCurveEvaluate(f32 T, v2 *P, u32 N)
{
 f32 H = 1.0f;
 f32 U = 1 - T;
 v2 Q = P[0];
 
 for (u32 K = 1; K < N; ++K)
 {
  H = H * T * (N - K);
  H = H / (K * U + H);
  Q = (1 - H) * Q + H * P[K];
 }
 
 return Q;
}

internal v2
BezierCurveRationalEvaluateScalar(f32 T, v2 *P, f32 *W, u32 N)
{
 f32 H = 1.0f;
 f32 U = 1 - T;
 v2 Q = P[0];
 
 for (u32 K = 1; K < N; ++K)
 {
  H = H * T * (N - K) * W[K];
  H = H / (K * U * W[K-1] + H);
  Q = (1 - H) * Q + H * P[K];
 }
 
 return Q;
}

internal void
BezierCurveRationalEvaluateSSE(f32 T[4], v2 *P, f32 *W, u32 N, v2 Out[4])
{
 __m128 t  = _mm_loadu_ps(T);          // [T0 T1 T2 T3]
 __m128 u  = _mm_sub_ps(_mm_set1_ps(1.0f), t); // U = 1 - T
 
 // H = 1
 __m128 H  = _mm_set1_ps(1.0f);
 
 // Q = P[0]
 __m128 Qx = _mm_set1_ps(P[0].X);
 __m128 Qy = _mm_set1_ps(P[0].Y);
 
 // Loop over K
 for (unsigned K = 1; K < N; ++K) {
  __m128 num = _mm_mul_ps(H, t);                     // H * T
  num = _mm_mul_ps(num, _mm_set1_ps((float)(N - K))); // * (N-K)
  num = _mm_mul_ps(num, _mm_set1_ps(W[K]));           // * W[K]
  
  __m128 denom = _mm_add_ps(
                            _mm_mul_ps(_mm_mul_ps(_mm_set1_ps((float)K), u), _mm_set1_ps(W[K-1])),
                            num
                            );
  
  H = _mm_div_ps(num, denom);
  
  // Q = (1 - H)*Q + H*P[K]
  __m128 one_minus_H = _mm_sub_ps(_mm_set1_ps(1.0f), H);
  __m128 Px = _mm_set1_ps(P[K].X);
  __m128 Py = _mm_set1_ps(P[K].Y);
  
  Qx = _mm_add_ps(_mm_mul_ps(one_minus_H, Qx), _mm_mul_ps(H, Px));
  Qy = _mm_add_ps(_mm_mul_ps(one_minus_H, Qy), _mm_mul_ps(H, Py));
 }
 
 // Store results back
 f32 out_x[4], out_y[4];
 _mm_storeu_ps(out_x, Qx); // writes [Q0.X Q1.X Q2.X Q3.X]
 _mm_storeu_ps(out_y, Qy); // writes [Q0.Y Q1.Y Q2.Y Q3.Y]
 
 for (int lane = 0; lane < 4; ++lane) {
  Out[lane].X = out_x[lane];
  Out[lane].Y = out_y[lane];
 }
}

internal void
BezierCurveRationalEvaluateAVX2(f32 T[8], v2 *P, f32 *W, u32 N, v2 Out[8])
{
 __m256 t  = _mm256_loadu_ps(T);
 __m256 u  = _mm256_sub_ps(_mm256_set1_ps(1.0f), t); // U = 1 - T
 
 // H = 1
 __m256 H  = _mm256_set1_ps(1.0f);
 
 // Q = P[0]
 __m256 Qx = _mm256_set1_ps(P[0].X);
 __m256 Qy = _mm256_set1_ps(P[0].Y);
 
 for (u32 K = 1; K < N; ++K) {
  __m256 num = _mm256_mul_ps(H, t);                            // H * T
  num = _mm256_mul_ps(num, _mm256_set1_ps((float)(N - K)));    // * (N-K)
  num = _mm256_mul_ps(num, _mm256_set1_ps(W[K]));              // * W[K]
  
  __m256 denom = _mm256_add_ps(
                               _mm256_mul_ps(
                                             _mm256_mul_ps(_mm256_set1_ps((float)K), u),
                                             _mm256_set1_ps(W[K - 1])
                                             ),
                               num
                               );
  
  H = _mm256_div_ps(num, denom);
  
  // Q = (1 - H)*Q + H*P[K]
  __m256 one_minus_H = _mm256_sub_ps(_mm256_set1_ps(1.0f), H);
  __m256 Px = _mm256_set1_ps(P[K].X);
  __m256 Py = _mm256_set1_ps(P[K].Y);
  
  Qx = _mm256_add_ps(_mm256_mul_ps(one_minus_H, Qx),
                     _mm256_mul_ps(H, Px));
  Qy = _mm256_add_ps(_mm256_mul_ps(one_minus_H, Qy),
                     _mm256_mul_ps(H, Py));
 }
 
 f32 out_x[8], out_y[8];
 _mm256_storeu_ps(out_x, Qx);
 _mm256_storeu_ps(out_y, Qy);
 
 for (u32 lane = 0; lane < 8; ++lane) {
  Out[lane].X = out_x[lane];
  Out[lane].Y = out_y[lane];
 }
}

internal void
BezierCurveRational_Evaluate_AVX512(f32 T[16], v2 *P, f32 *W, u32 N, v2 Out[16])
{
 __m512 t  = _mm512_loadu_ps(T);
 __m512 u  = _mm512_sub_ps(_mm512_set1_ps(1.0f), t);
 __m512 H  = _mm512_set1_ps(1.0f);
 
 __m512 Qx = _mm512_set1_ps(P[0].X);
 __m512 Qy = _mm512_set1_ps(P[0].Y);
 
 for (u32 K = 1; K < N; ++K) {
  __m512 num = _mm512_mul_ps(H, t);
  num = _mm512_mul_ps(num, _mm512_set1_ps((float)(N - K)));
  num = _mm512_mul_ps(num, _mm512_set1_ps(W[K]));
  
  __m512 denom = _mm512_add_ps(
                               _mm512_mul_ps(
                                             _mm512_mul_ps(_mm512_set1_ps((float)K), u),
                                             _mm512_set1_ps(W[K - 1])),
                               num
                               );
  
  H = _mm512_div_ps(num, denom);
  
  __m512 one_minus_H = _mm512_sub_ps(_mm512_set1_ps(1.0f), H);
  __m512 Px = _mm512_set1_ps(P[K].X);
  __m512 Py = _mm512_set1_ps(P[K].Y);
  
  Qx = _mm512_add_ps(_mm512_mul_ps(one_minus_H, Qx), _mm512_mul_ps(H, Px));
  Qy = _mm512_add_ps(_mm512_mul_ps(one_minus_H, Qy), _mm512_mul_ps(H, Py));
 }
 
 f32 out_x[8], out_y[8];
 _mm512_storeu_ps(out_x, Qx);
 _mm512_storeu_ps(out_y, Qy);
 
 for (u32 lane = 0; lane < 8; ++lane) {
  Out[lane].X = out_x[lane];
  Out[lane].Y = out_y[lane];
 }
}

internal void
BezierCurve_ElevateDegree(v2 *P, u32 N)
{
 if (N > 0)
 {
  v2 PN = P[N-1];
  f32 Inv_N = 1.0f / N;
  for (u32 I = N-1; I >= 1; --I)
  {
   f32 Mix= I * Inv_N;
   P[I] = Mix * P[I-1] + (1 - Mix) * P[I];
  }
  
  P[N] = PN;
 }
}

internal void
BezierCurveRational_EvelateDegree(v2 *P, f32 *W, u32 N)
{
 if (N > 0)
 {
  f32 WN = W[N-1];
  v2 PN = P[N-1];
  f32 Inv_N = 1.0f / N;
  for (u32 I = N-1; I >= 1; --I)
  {
   f32 Mix = I * Inv_N;
   
   f32 Left_W = Mix * W[I-1];
   f32 Right_W = (1 - Mix) * W[I];
   f32 New_W = Left_W + Right_W;
   
   f32 Inv_New_W = 1.0f / New_W;
   P[I] = Left_W * Inv_New_W * P[I-1] + Right_W * Inv_New_W * P[I]; 
   W[I] = New_W;
  }
  
  P[N] = PN;
  W[N] = WN;
 }
}

#if 1
// NOTE(hbr): Optimized, O(1) memory version
internal bezier_lower_degree_inverse_degree_elevation
BezierCurveLowerDegreeUsingInverseDegreeElevation(v2 *P, f32 *W, u32 N)
{
 bezier_lower_degree_inverse_degree_elevation Result = {};
 
 if (N >= 2)
 {
  u32 H = ((N-1) >> 1) + 1;
  
  f32 Prev_Front_W = 0.0f;
  v2 Prev_Front_P = {};
  for (u32 K = 0; K < H; ++K)
  {
   f32 Alpha = Cast(f32)K / (N-1-K);
   
   f32 Left_W = (1 + Alpha) * W[K];
   f32 Right_W = Alpha * Prev_Front_W;
   f32 New_W = Left_W - Right_W;
   
   f32 Inv_New_W = 1.0f / New_W;
   v2 New_P = Left_W * Inv_New_W * P[K] - Right_W * Inv_New_W * Prev_Front_P;
   
   W[K] = New_W;
   P[K] = New_P;
   
   Prev_Front_W = New_W;
   Prev_Front_P = New_P;
  }
  // NOTE(hbr): Prev_Front_W == W_I[H-1] and Prev_Front_P == P_I[H-1]
  // at this point
  
  f32 Prev_Back_W = 0.0f;
  v2 Prev_Back_P = {};
  f32 Save_W = W[N-1];
  v2 Save_P = P[N-1];
  for (u32 K = N-1; K >= H; --K)
  {
   f32 Alpha = Cast(f32)(N-1) / K;
   
   f32 Left_W = Alpha * Save_W;
   f32 Right_W = (1 - Alpha) * Prev_Back_W;
   f32 New_W = Left_W + Right_W;
   
   f32 Inv_New_W = 1.0f / New_W;
   v2 New_P = Left_W * Inv_New_W * Save_P + Right_W * Inv_New_W * Prev_Back_P;
   
   Save_W = W[K-1];
   Save_P = P[K-1];
   
   W[K-1] = New_W;
   P[K-1] = New_P;
   
   Prev_Back_W = New_W;
   Prev_Back_P = New_P;
  }
  // NOTE(hbr): Prev_Back_W == W_II[H-1] and Prev_Back_P == P_II[H-1]
  // at this point
  
  if (!ApproxEq32(Prev_Front_P.X, Prev_Back_P.X) ||
      !ApproxEq32(Prev_Front_P.Y, Prev_Back_P.Y))
  {
   Result.Failure = true;
   Result.MiddlePointIndex = H-1;
   Result.P_I = Prev_Front_P;
   Result.P_II = Prev_Back_P;
   Result.W_I = ClampBot(Prev_Front_W, F32_EPS);
   Result.W_II = ClampBot(Prev_Back_W, F32_EPS);
  }
 }
 
 return Result;
}
#else
// NOTE(hbr): Non-optimzed, O(N) memory version for reference
internal void
BezierCurveLowerDegreeUsingInverseDegreeElevation(v2 *P, f32 *W, u32 N)
{
 if (N >= 1)
 {
  temp_arena Temp = TempArena(0);
  defer { EndTemp(Temp); };
  
  f32 *Front_W = PushArrayNonZero(Temp.Arena, N-1, f32);
  f32 *Back_W = PushArrayNonZero(Temp.Arena, N-1, f32);
  
  {
   f32 Prev_Front_W = 0.0f;
   for (u32 K = 0; K < N-1; ++K)
   {
    f32 Alpha = Cast(f32)K / (N-1-K);
    Front_W[K] = (1 + Alpha) * W[K] - Alpha * Prev_Front_W;
    Prev_Front_W = Front_W[K];
   }
  }
  
  {   
   f32 Prev_Back_W = 0.0f;
   for (u32 K = N-1; K >= 1; --K)
   {
    f32 Alpha = Cast(f32)(N-1-K) / K;
    Back_W[K-1] = (1 + Alpha) * W[K] - Alpha * Prev_Back_W;
    Prev_Back_W = Back_W[K-1];
   }
  }
  
  v2 *Front_P = PushArrayNonZero(Temp.Arena, N-1, v2);
  v2 *Back_P = PushArrayNonZero(Temp.Arena, N-1, v2);
  
  {   
   f32 Prev_Front_W = 0.0f;
   v2 Prev_Front_P = {};
   for (u32 K = 0; K < N-1; ++K)
   {
    f32 Alpha = Cast(f32)K / (N-1-K);
    Front_P[K] = (1 + Alpha) * W[K]/Front_W[K] * P[K] - Alpha * Prev_Front_W/Front_W[K] * Prev_Front_P;
    Prev_Front_P = Front_P[K];
    Prev_Front_W = Front_W[K];
   }
  }
  
  {
   f32 Prev_Back_W = 0.0f;
   v2 Prev_Back_P = {};
   for (u32 K = N-1; K >= 1; --K)
   {
    f32 Alpha = Cast(f32)(N-1-K) / K;
    Back_P[K-1] = (1 + Alpha) * W[K]/Back_W[K-1] * P[K] - Alpha * Prev_Back_W/Back_W[K-1] * Prev_Back_P;
    Prev_Back_W = Back_W[K-1];
    Prev_Back_P = Back_P[K-1];
   }
  }
  
  u32 H = (N >> 1) + 1;
  MemoryCopy(W, Front_W, H * SizeOf(W[0]));
  MemoryCopy(W+H, Back_W+H, (N-1-H) * SizeOf(W[0]));
  MemoryCopy(P, Front_P, H * SizeOf(P[0]));
  MemoryCopy(P+H, Back_P+H, (N-1-H) * SizeOf(P[0]));
  
  local f32 Mix = 0.5f;
  W[H-1] = Mix * Front_W[H-1] + (1 - Mix) * Back_W[H-1];
  P[H-1] = Mix * Front_P[H-1] + (1 - Mix) * Back_P[H-1];
 }
}
#endif

internal f32 *
CalculateChebyshevPolynomial01(arena *Arena, u32 n)
{
 f32 *T = PushArrayNonZero(Arena, n + 1, f32);
 
 if (n == 0)
 {
  T[0] = 1;
 }
 else if (n == 1)
 {
  T[0] = -1;
  T[1] = 2;
 }
 else
 {
  temp_arena Temp = TempArena(Arena);
  
  f32 *T_1 = CalculateChebyshevPolynomial01(Temp.Arena, n-1);
  f32 *T_2 = CalculateChebyshevPolynomial01(Temp.Arena, n-2);
  
  for (u32 i = 0; i <= n; ++i)
  {
#define SafeAccess(A, N, I) (I < N ? A[I] : 0)
   f32 A = (i > 0 ? SafeAccess(T_1, n-1+1, i-1) : 0);
   f32 B = SafeAccess(T_1, n-1+1, i);
   f32 C = SafeAccess(T_2, n-2+1, i);
   T[i] = 4*A - 2*B - C;
#undef SafeAccess
  }
  
  EndTemp(Temp);
 }
 
 return T;
}

internal u64
SafeMultU64(u64 a, u64 b)
{
 if (a != 0 && b > U64_MAX / a)
 {
  Assert(!"Overflow!");
 }
 u64 r = a * b;
 return r;
}

internal u64
Factorial(u64 n)
{
 u64 f = 1;
 for (u64 i = 1; i <= n; ++i)
 {
  f = SafeMultU64(f, i);
 }
 return f;
}

internal u64
PowInteger(u64 a, u64 k)
{
 u64 r = 1;
 while (k)
 {
  if (k & 1) r = SafeMultU64(r, a);
  a = SafeMultU64(a, a);
  k <<= 1;
 }
 return r;
}

internal f32
GeneralBinomial(f32 a, u32 k)
{
 f32 r = 1.0f;
 f32 x = a;
 for (u32 i = 0; i < k; ++i)
 {
  r *= x;
  x -= 1;
 }
 r /= Factorial(k);
 return r;
}

internal f32
MinusOnePow(u32 n)
{
 f32 r = 0;
 if (n & 1) r = -1.0f;
 else r = 1.0f;
 return r;
}

internal f32 *
CalculateChebyshevPolynomialThroughBernstein(arena *Arena, u32 n)
{
 f32 *w = PushArray(Arena, n+1, f32);
 
 u64 n_fact = Factorial(n);
 u64 two_n_fact = Factorial(2*n);
 
 f32 n_fact_f = Cast(f32)n_fact;
 f32 two_n_fact_f = Cast(f32)two_n_fact;
 
 f32 c_outer = PowF32(2.0f, Cast(f32)(2*n)) * n_fact * n_fact / two_n_fact;
 
 for (u32 i = 0; i <= n; ++i)
 {
  f32 minus_one_pow = MinusOnePow(n - i);
  f32 binom_n_2_i = GeneralBinomial(n - 0.5f, i);
  f32 binom_n_2_n_i = GeneralBinomial(n - 0.5f, n - i);
  f32 binom_n_i = GeneralBinomial(Cast(f32)n, i);
  f32 beta_i = minus_one_pow * binom_n_2_i * binom_n_2_n_i / binom_n_i;
  
  for (u32 j = 0; j <= n-i; ++j)
  {
   f32 binom_n_i_j = GeneralBinomial(Cast(f32)(n-i), j);
   f32 minus_one_n_i_j = MinusOnePow(n-i-j);
   
   w[n-j] += c_outer * beta_i * binom_n_i  * binom_n_i_j * minus_one_n_i_j;
  }
 }
 
 return w;
}

internal f32 *
ConvertBernsteinIntoStandardBasis(arena *Arena, f32 *B, u32 N)
{
 f32 *S = PushArray(Arena, N, f32);
 u32 n = N-1;
 for (u32 i = 0; i <= n; ++i)
 {
  for (u32 j = 0; j <= n-i; ++j)
  {
   f32 binom_n_i = GeneralBinomial(Cast(f32)n, i);
   f32 binom_n_i_j = GeneralBinomial(Cast(f32)(n - i), j);
   f32 minus_one_n_i_j = MinusOnePow(n - i - j);
   f32 coeff = binom_n_i * binom_n_i_j * minus_one_n_i_j * B[i];
   S[n - j] += coeff;
  }
 }
 
 return S;
}

internal v2
CalculateAlphaN(v2 *P, u32 N)
{
 u32 n = N-1;
 v2 alpha_n = V2(0, 0);
 f32 coeff = 1.0f;
 for (u32 idx = 0; idx < N; ++idx)
 {
  u32 i = n-idx;
  alpha_n += P[i] * coeff;
  coeff *= (-1);
  coeff *= i;
  coeff /= (n-i+1);
 }
 alpha_n = alpha_n / PowerOf2(2*n - 1);
 
 return alpha_n;
}

internal void
BezierCurveLowerDegreeUniformNormOptimal(v2 *P, f32 *W, u32 N)
{
 if (N > 0)
 {
  u32 n = N - 1;
  
  // calculate c_n := (n!)^2 / (2n)!
  f32 c_n = 2.0f;
  for (u32 i = 1; i <= n; ++i)
  {
   c_n *= Cast(f32)i / (n + i);
  }
  
  // calculate alpha_n := sum_(i=0,..n) P_i binom(n, i) (-1)^(n-i)
  v2 alpha_n = V2(0, 0);
  f32 coeff = 1.0f;
  for (u32 idx = 0; idx <= n; ++idx)
  {
   u32 i = n - idx;
   alpha_n += coeff * P[i];
   coeff *= -Cast(f32)i / (n - i + 1);
  }
  
  // calculate beta_n' := binom(n-1/2, n)
  f32 beta_n_prim = 1.0f;
  for (u32 i = 1; i <= n; ++i)
  {
   beta_n_prim *=  Cast(f32)(i - 0.5f) / i;
  }
  
  f32 beta_i = c_n * beta_n_prim;
  for (u32 idx = 0; idx <= n; ++idx)
  {
   u32 i = n - idx;
   P[i] -= beta_i * alpha_n;
   beta_i *= (-1) * (i - 0.5f) / (n - i + 0.5f);
  }
  
  bezier_lower_degree_inverse_degree_elevation Lower = BezierCurveLowerDegreeUsingInverseDegreeElevation(P, W, N);
  //Assert(!Lower.Failure);
 }
}

internal inline void
CalculateBezierCubicPointAt(u32 N, v2 *P, cubic_bezier_point *Out, u32 At)
{
 // NOTE(hbr): All of these equations assume that all t_i are equidistant.
 // Otherwise equations are more complicated. Either way, this is the current
 // state of t_i - they are equidistant.
#define w(i) (i < N ? P[i] : V2(0, 0)) 
 
#define s_(i) (0.5f*c(i-1) + 0.5f*c(i))
#define s(i) \
(i == 0 ? \
(2*c(0) - s_(1)) : \
((i == N-1) ? \
(2*c(N-2) - s_(N-2)) : \
s_(i)))
 
#define c(i) (SafeDiv0(1, dt) * (w(i+1) - w(i)))
 
 f32 dt = SafeDiv0(1.0f, N-1);
 u32 i = At;
 
 Out[At].P0 = (N > 2 ? (w(i) - 1.0f/3.0f * dt * s(i)) : w(i));
 Out[At].P1 = w(i);
 Out[At].P2 = (N > 2 ? (w(i) + 1.0f/3.0f * dt * s(i)) : w(i));
 
#undef s
#undef s_
#undef c
#undef w
}

internal void
BezierCubicCalculateAllControlPoints(u32 N, v2 *P, cubic_bezier_point *Out)
{
 for (u32 I = 0; I < N; ++I)
 {
  CalculateBezierCubicPointAt(N, P, Out, I);
 }
}

internal void
BezierCurveSplit(f32 T, u32 N, v2 *P, f32 *W, 
                 v2 *LeftPoints, f32 *LeftWeights,
                 v2 *RightPoints, f32 *RightWeights)
{
 temp_arena Temp = TempArena(0);
 
 all_de_casteljau_intermediate_results Intermediate = DeCasteljauAlgorithm(Temp.Arena, T, P, W, N);
 
 {   
  u32 Index = 0;
  for (u32 I = 0; I < N; ++I)
  {
   LeftPoints[I] = Intermediate.P[Index];
   LeftWeights[I] = Intermediate.W[Index];
   Index += N - I;
  }
 }
 
 {
  u32 Index = Intermediate.TotalPointCount - 1;
  for (u32 I = 0; I < N; ++I)
  {
   RightPoints[I] = Intermediate.P[Index];
   RightWeights[I] = Intermediate.W[Index];
   Index -= I + 1;
  }
 }
 
 EndTemp(Temp);
}

internal all_de_casteljau_intermediate_results
DeCasteljauAlgorithm(arena *Arena, f32 T, v2 *P, f32 *W, u32 N)
{
 all_de_casteljau_intermediate_results Result = {};
 
 u32 TotalPointCount = N * (N+1) / 2;
 v2 *OutP = PushArrayNonZero(Arena, TotalPointCount, v2);
 f32   *OutW = PushArrayNonZero(Arena, TotalPointCount, f32);
 
 MemoryCopy(OutW, W, N * SizeOf(OutW[0]));
 MemoryCopy(OutP, P, N * SizeOf(OutP[0]));
 
 u32 Index = N;
 for (u32 Iteration = 1;
      Iteration < N;
      ++Iteration)
 {
  for (u32 J = 0; J < N-Iteration; ++J)
  {
   u32 IndexLeft = Index - (N-Iteration)-1;
   u32 IndexRight = Index - (N-Iteration);
   
   f32 WeightLeft = (1-T) * OutW[IndexLeft];
   f32 WeightRight = T * OutW[IndexRight];
   
   f32 Weight = WeightLeft + WeightRight;
   f32 InvWeight = 1.0f / Weight;
   
   OutW[Index] = Weight;
   OutP[Index] = ((WeightLeft*InvWeight) * OutP[IndexLeft] +
                  (WeightRight*InvWeight) * OutP[IndexRight]);
   
   ++Index;
  }
 }
 Assert(Index == TotalPointCount);
 
 Result.IterationCount = N;
 Result.TotalPointCount = TotalPointCount;
 Result.P = OutP;
 Result.W = OutW;
 
 return Result;
}

internal f32
PointDistanceSquaredSigned(v2 P, v2 Point, f32 Radius)
{
 f32 Result = NormSquared(P - Point) - Radius*Radius;
 return Result;
}

internal b32
PointCollision(v2 Position, v2 Point, f32 PointRadius)
{
 v2 Delta = Position - Point;
 b32 Collision = ((Abs(Delta.X) <= PointRadius) && (Abs(Delta.Y) <= PointRadius));
 
 return Collision;
}

internal f32
AABBSignedDistance(v2 P, v2 BoxP, v2 BoxSize)
{
 v2 HalfSize = 0.5f * BoxSize;
 
 f32 Left = BoxP.X - HalfSize.X;
 f32 Right = BoxP.X + HalfSize.X;
 f32 Bottom = BoxP.Y - HalfSize.Y;
 f32 Top = BoxP.Y + HalfSize.Y;
 
 f32 DistX = Max(Left-P.X, P.X-Right);
 f32 DistY = Max(Bottom-P.Y, P.Y-Top);
 f32 Dist = Max(DistX, DistY);
 
 return Dist;
}

internal f32
SegmentSignedDistance(v2 P, v2 SegmentBegin, v2 SegmentEnd, f32 SegmentWidth)
{
 rotation2d Rotation = Rotation2DFromVector(SegmentEnd - SegmentBegin);
 rotation2d InvRotation = Rotation2DInverse(Rotation);
 v2 SegmentEndAligned = RotateAround(SegmentEnd, SegmentBegin, InvRotation);
 v2 SegmentSize = V2(Abs(SegmentEndAligned.X - SegmentBegin.X), SegmentWidth);
 
 v2 SegmentMid = SegmentBegin;
 SegmentMid.X += 0.5f * SegmentSize.X;
 
 v2 PAligned = RotateAround(P, SegmentBegin, InvRotation);
 
 f32 Result = AABBSignedDistance(PAligned, SegmentMid, SegmentSize);
 
 return Result;
}

internal b32
SegmentCollision(v2 P, v2 LineA, v2 LineB, f32 LineWidth)
{
 b32 Result = false;
 
 v2 U = LineA - LineB;
 v2 V = P - LineB;
 
 f32 SegmentLengthSquared = Dot(U, U);
 f32 DotUV = Dot(U, V);
 if (DotUV >= 0.0f)
 {
  f32 Inv_SegmentLengthSquared = 1.0f / SegmentLengthSquared;
  f32 ProjectionLengthSquared = Sqr(DotUV) * Inv_SegmentLengthSquared;
  
  if (ProjectionLengthSquared <= DotUV)
  {
   // |Ax + By + C| / sqrt(A^2 + B^2)
   f32 A = U.Y;
   f32 B = -U.X;
   f32 C = -A*LineA.X - B*LineA.Y;
   
   f32 LineDist = Abs(A*P.X + B*P.Y + C) * SqrtF32(Inv_SegmentLengthSquared);
   
   Result = (LineDist <= 0.5f * LineWidth);
  }
 }
 
 return Result;
}

internal line_intersection
LineIntersection(v2 A, v2 B, v2 C, v2 D)
{
 line_intersection Result = {};
 
 f32 X1 = A.X, Y1 = A.Y;
 f32 X2 = B.X, Y2 = B.Y;
 f32 X3 = C.X, Y3 = C.Y;
 f32 X4 = D.X, Y4 = D.Y;
 
 f32 Det = (X1-X2) * (Y3-Y4) - (Y1-Y2) * (X3-X4);
 if (!ApproxEq32(Det, 0.0f))
 {
  Result.IsOneIntersection = true;
  f32 X = (X1*Y2 - Y1*X2) * (X3-X4) - (X1-X2) * (X3*Y4 - Y3*X4);
  f32 Y = (X1*Y2 - Y1*X2) * (Y3-Y4) - (Y1-Y2) * (X3*Y4 - Y3*X4);
  Result.IntersectionPoint = V2(X/Det, Y/Det);
 }
 
 return Result;
}

internal inline v2
operator*(mat3 A, v2 P)
{
 v2 R = Transform3x3(A, V3(P, 1.0f)).XY;
 return R;
}

internal inline v3
operator*(mat4 A, v3 P)
{
 v3 R = Transform4x4(A, V4(P, 1.0f)).XYZ;
 return R;
}

internal inline v3
operator*(mat3 A, v3 P)
{
 v3 R = Transform3x3(A, P);
 return R;
}

internal inline v4
operator*(mat4 A, v4 P)
{
 v4 R = Transform4x4(A, P);
 return R;
}

internal mat3
Multiply3x3(mat3 A, mat3 B)
{
 mat3 R;
 
 R.M[0][0] = A.M[0][0]*B.M[0][0] + A.M[0][1]*B.M[1][0] + A.M[0][2]*B.M[2][0];
 R.M[0][1] = A.M[0][0]*B.M[0][1] + A.M[0][1]*B.M[1][1] + A.M[0][2]*B.M[2][1];
 R.M[0][2] = A.M[0][0]*B.M[0][2] + A.M[0][1]*B.M[1][2] + A.M[0][2]*B.M[2][2];
 
 R.M[1][0] = A.M[1][0]*B.M[0][0] + A.M[1][1]*B.M[1][0] + A.M[1][2]*B.M[2][0];
 R.M[1][1] = A.M[1][0]*B.M[0][1] + A.M[1][1]*B.M[1][1] + A.M[1][2]*B.M[2][1];
 R.M[1][2] = A.M[1][0]*B.M[0][2] + A.M[1][1]*B.M[1][2] + A.M[1][2]*B.M[2][2];
 
 R.M[2][0] = A.M[2][0]*B.M[0][0] + A.M[2][1]*B.M[1][0] + A.M[2][2]*B.M[2][0];
 R.M[2][1] = A.M[2][0]*B.M[0][1] + A.M[2][1]*B.M[1][1] + A.M[2][2]*B.M[2][1];
 R.M[2][2] = A.M[2][0]*B.M[0][2] + A.M[2][1]*B.M[1][2] + A.M[2][2]*B.M[2][2];
 
 return R;
}

internal mat3
operator*(mat3 A, mat3 B)
{
 mat3 R = Multiply3x3(A, B);
 return R;
}

internal mat3_inv
CameraTransform(v2 P, rotation2d Rotation, f32 Zoom)
{
 mat3_inv Result = {};
 
 rotation2d XAxis = Rotation;
 rotation2d YAxis = Rotate90DegreesAntiClockwise(Rotation);
 
 // TODO(hbr): It isn't strictly correct order to first do Rows3x3 and then Scale3x3.
 // It should be the other way around. But we scale uniformly by scalar anyway so it
 // doesn't matter.
 mat3 A = Rows3x3(XAxis.V, YAxis.V);
 A = Scale3x3(A, Zoom);
 v2 AP = -(A*P);
 A = Translate3x3(A, AP);
 Result.Forward = A;
 
 mat3 iA = Cols3x3(XAxis.V, YAxis.V);
 iA = Scale3x3(iA, 1 / Zoom);
 iA = Translate3x3(iA, P);
 Result.Inverse = iA;
 
 return Result;
}

internal mat3_inv
ClipTransform(f32 AspectRatio)
{
 mat3_inv R;
 R.Forward = Diag3x3(1/AspectRatio, 1.0f);
 R.Inverse = Diag3x3(AspectRatio, 1.0f);
 
 return R;
}

internal mat3
ModelTransform(v2 P, rotation2d Rotation, scale2d Scale)
{
 rotation2d XAxis = Rotation;
 rotation2d YAxis = Rotate90DegreesAntiClockwise(Rotation);
 
 // NOTE(hbr): First scale, then rotate, then translate.
 // It's crucial to do that in that order
 mat3 A = Identity3x3();
 A = Scale3x3(A, Scale.V);
 A = Cols3x3(XAxis.V, YAxis.V) * A;
 A = Translate3x3(A, P);
 
 return A;
}

internal v2
Unproject(render_group *RenderGroup, v2 Clip)
{
 v2 R = RenderGroup->ProjXForm.Inverse * Clip;
 return R;
}

internal mat3
Identity3x3(void)
{
 mat3 Result = {};
 Result.M[0][0] = 1.0f;
 Result.M[1][1] = 1.0f;
 Result.M[2][2] = 1.0f;
 
 return Result;
}

internal mat4
Identity4x4(void)
{
 mat4 Result = {};
 Result.M[0][0] = 1.0f;
 Result.M[1][1] = 1.0f;
 Result.M[2][2] = 1.0f;
 Result.M[3][3] = 1.0f;
 
 return Result;
}

internal mat3
Transpose3x3(mat3 M)
{
 mat3 R = {
  { {M.M[0][0], M.M[1][0], M.M[2][0]},
   {M.M[0][1], M.M[1][1], M.M[2][1]},
   {M.M[0][2], M.M[1][2], M.M[2][2]}}
 };
 return R;
}

internal mat4
Transpose4x4(mat4 M)
{
 mat4 R = {
  { {M.M[0][0], M.M[1][0], M.M[2][0], M.M[3][0]},
   {M.M[0][1], M.M[1][1], M.M[2][1], M.M[3][1]},
   {M.M[0][2], M.M[1][2], M.M[2][2], M.M[3][2]},
   {M.M[0][3], M.M[1][3], M.M[2][3], M.M[3][3]}}
 };
 return R;
}

internal mat3
Rows3x3(v2 X, v2 Y)
{
 mat3 R = {
  { { X.X, X.Y, 0 },
   { Y.X, Y.Y, 0 },
   {   0,   0, 1 }}
 };
 return R;
}

internal mat3
Cols3x3(v2 X, v2 Y)
{
 mat3 R = {
  { { X.X, Y.X, 0},
   { X.Y, Y.Y, 0},
   {   0,   0, 1}}
 };
 return R;
}

internal mat3
Scale3x3(mat3 A, f32 Scale)
{
 mat3 R = A;
 
 R.M[0][0] *= Scale;
 R.M[0][1] *= Scale;
 R.M[1][0] *= Scale;
 R.M[1][1] *= Scale;
 
 return R;
}

internal mat4
Scale4x4(mat4 A, f32 Scale)
{
 mat4 R = A;
 
 R.M[0][0] *= Scale;
 R.M[0][1] *= Scale;
 R.M[0][2] *= Scale;
 R.M[1][0] *= Scale;
 R.M[1][1] *= Scale;
 R.M[1][2] *= Scale;
 R.M[2][0] *= Scale;
 R.M[2][1] *= Scale;
 R.M[2][2] *= Scale;
 
 return R;
}

internal mat4
Scale4x4(mat4 A, v3 Scale)
{
 mat4 R = A;
 
 R.M[0][0] *= Scale.X;
 R.M[0][1] *= Scale.X;
 R.M[0][2] *= Scale.X;
 R.M[1][0] *= Scale.Y;
 R.M[1][1] *= Scale.Y;
 R.M[1][2] *= Scale.Y;
 R.M[2][0] *= Scale.Z;
 R.M[2][1] *= Scale.Z;
 R.M[2][2] *= Scale.Z;
 
 return R;
}

internal mat3
Scale3x3(mat3 A, v2 Scale)
{
 mat3 R = A;
 
 R.M[0][0] *= Scale.X;
 R.M[0][1] *= Scale.X;
 R.M[1][0] *= Scale.Y;
 R.M[1][1] *= Scale.Y;
 
 return R;
}

internal mat3
Translate3x3(mat3 A, v2 P)
{
 mat3 R = A;
 
 R.M[0][2] += P.X;
 R.M[1][2] += P.Y;
 
 return R;
}

internal mat4
Translate4x4(mat4 A, v3 P)
{
 mat4 R = A;
 
 R.M[0][3] += P.X;
 R.M[1][3] += P.Y;
 R.M[2][3] += P.Z;
 
 return R;
}

internal v3
Transform3x3(mat3 A, v3 P)
{
 v3 R = {
  P.X*A.M[0][0] + P.Y*A.M[0][1] + P.Z*A.M[0][2],
  P.X*A.M[1][0] + P.Y*A.M[1][1] + P.Z*A.M[1][2],
  P.X*A.M[2][0] + P.Y*A.M[2][1] + P.Z*A.M[2][2],
 };
 return R;
}

internal v4
Transform4x4(mat4 A, v4 P)
{
 v4 R = {
  P.X*A.M[0][0] + P.Y*A.M[0][1] + P.Z*A.M[0][2] + P.W*A.M[0][3],
  P.X*A.M[1][0] + P.Y*A.M[1][1] + P.Z*A.M[1][2] + P.W*A.M[1][3],
  P.X*A.M[2][0] + P.Y*A.M[2][1] + P.Z*A.M[2][2] + P.W*A.M[2][3],
  P.X*A.M[3][0] + P.Y*A.M[3][1] + P.Z*A.M[3][2] + P.W*A.M[3][3],
 };
 return R;
}

internal mat3
Diag3x3(f32 X, f32 Y)
{
 mat3 R = {
  { {X, 0, 0},
   {0, Y, 0},
   {0, 0, 1}}
 };
 return R;
}

internal f32
TriangleArea(v2 P0, v2 P1, v2 P2)
{
 f32 C = Cross(P1 - P0, P2 - P0);
 f32 Area = Abs(0.5f * C);
 return Area;
}

internal samples
MakeSamples(void)
{
 samples Samples = {};
 Samples.SamplesMin = F32_MAX;
 Samples.SamplesMax = F32_MIN;
 return Samples;
}

internal void
AddSample(samples *Set, f32 Sample)
{
 Set->SamplesMin = Min(Set->SamplesMin, Sample);
 Set->SamplesMax = Max(Set->SamplesMax, Sample);
 Set->SamplesSum += Sample;
 Set->SamplesSum2 += Sample*Sample;
 ++Set->Count;
 Set->SamplesAvg = SafeDiv0(Set->SamplesSum, Set->Count);
 Set->SamplesStddev = SqrtF32(SafeDiv0(Set->SamplesSum2 - 2*Set->SamplesAvg*Set->SamplesSum, Set->Count) + Set->SamplesAvg);
}
