internal v4
RGBA_Color(u8 R, u8 G, u8 B, u8 A)
{
 v4 Result = {};
 Result.R = R / 255.0f;
 Result.G = G / 255.0f;
 Result.B = B / 255.0f;
 Result.A = A / 255.0f;
 
 return Result;
}

internal v4
BrightenColor(v4 Color, f32 BrightenByRatio)
{
 v4 Result = {};
 Result.R = (1.0f + BrightenByRatio) * Color.R;
 Result.G = (1.0f + BrightenByRatio) * Color.G;
 Result.B = (1.0f + BrightenByRatio) * Color.B;
 Result.A = Color.A;
 
 return Result;
}

inline internal v4
DarkenColor(v4 Color, f32 DarkenByRatio)
{
 v4 Result = BrightenColor(Color, -DarkenByRatio);
 return Result;
}

inline internal v4
FadeColor(v4 Color, f32 FadeByRatio)
{
 v4 Result = Color;
 Result.A *= FadeByRatio;
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
Rotation2D(f32 X, f32 Y)
{
 v2 Rotation = {};
 Rotation.X = X;
 Rotation.Y = Y;
 
 return Rotation;
}

internal v2
Rotation2DZero(void)
{
 v2 Result = Rotation2D(1.0f, 0.0f);
 return Result;
}

internal v2
Rotation2DFromVector(v2 Vector)
{
 Normalize(&Vector);
 v2 Result = Rotation2D(Vector.X, Vector.Y);
 
 return Result;
}

internal v2
Rotation2DFromDegrees(f32 Degrees)
{
 f32 Radians = DegToRad32 * Degrees;
 v2 Result = Rotation2DFromRadians(Radians);
 
 return Result;
}

internal v2
Rotation2DFromRadians(f32 Radians)
{
 v2 Result = Rotation2D(CosF32(Radians), SinF32(Radians));
 return Result;
}

internal f32
Rotation2DToDegrees(v2 Rotation)
{
 f32 Radians = Rotation2DToRadians(Rotation);
 f32 Degrees = RadToDeg32 * Radians;
 
 return Degrees;
}

internal f32
Rotation2DToRadians(v2 Rotation)
{
 f32 Radians = Atan2F32(Rotation.Y, Rotation.X);
 return Radians;
}

internal v2
Rotation2DInverse(v2 Rotation)
{
 v2 InverseRotation = Rotation2D(Rotation.X, -Rotation.Y);
 return InverseRotation;
}

internal v2
Rotate90DegreesAntiClockwise(v2 Rotation)
{
 v2 Result = Rotation2D(-Rotation.Y, Rotation.X);
 return Result;
}

internal v2
Rotate90DegreesClockwise(v2 Rotation)
{
 v2 Result = Rotation2D(Rotation.Y, -Rotation.X);
 return Result;
}

internal v2
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
 
 v2 Rotation = Rotation2D(Cos, Sin);
 return Rotation;
}

internal v2
RotateAround(v2 Point, v2 Center, v2 Rotation)
{
 v2 Translated = Point - Center;
 v2 Rotated = V2(Rotation.X * Translated.X - Rotation.Y * Translated.Y,
                 Rotation.X * Translated.Y + Rotation.Y * Translated.X);
 v2 Result = Rotated + Center;
 
 return Result;
}

internal v2
CombineRotations2D(v2 RotationA, v2 RotationB)
{
 f32 RotationX = RotationA.X * RotationB.X - RotationA.Y * RotationB.Y;
 f32 RotationY = RotationA.X * RotationB.Y + RotationA.Y * RotationB.X;
 v2 RotationVector = V2(RotationX, RotationY);
 Normalize(&RotationVector);
 
 v2 Result = Rotation2D(RotationVector.X, RotationVector.Y);
 return Result;
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
  v2 NV_Line = Rotate90DegreesAntiClockwise(V_Line);
  Normalize(&NV_Line);
  
  Vertices[VertexIndex + 0] = (A + 0.5f * Width * NV_Line);
  Vertices[VertexIndex + 1] = (A - 0.5f * Width * NV_Line);
  
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
  v2 NV_Line = Rotate90DegreesAntiClockwise(V_Line);
  
  v2 V_Succ = C - B;
  Normalize(&V_Succ);
  v2 NV_Succ = Rotate90DegreesAntiClockwise(V_Succ);
  
  b32 LeftTurn = (Cross(V_Line, V_Succ) >= 0.0f);
  f32 TurnedHalfWidth = (LeftTurn ? 1.0f : -1.0f) * 0.5f * Width;
  
  line_intersection Intersection = LineIntersection(A + TurnedHalfWidth * NV_Line,
                                                    B + TurnedHalfWidth * NV_Line,
                                                    B + TurnedHalfWidth * NV_Succ,
                                                    C + TurnedHalfWidth * NV_Succ);
  
  v2 IntersectionPoint = {};
  if (Intersection.IsOneIntersection)
  {
   IntersectionPoint = Intersection.IntersectionPoint;
  }
  else
  {
   IntersectionPoint = B + TurnedHalfWidth * NV_Line;
  }
  
  v2 B_Line = B - TurnedHalfWidth * NV_Line;
  v2 B_Succ = B - TurnedHalfWidth * NV_Succ;
  
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
   v2 NV_Line = Rotate90DegreesAntiClockwise(V_Line);
   Normalize(&NV_Line);
   
   v2 B_Inside  = B + 0.5f * Width * NV_Line;
   v2 B_Outside = B - 0.5f * Width * NV_Line;
   
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

internal v4
LerpColor(v4 From, v4 To, f32 T)
{
 v4 Result = {};
 Result.R = Cast(u8)((1-T) * From.R + T * To.R);
 Result.G = Cast(u8)((1-T) * From.G + T * To.G);
 Result.B = Cast(u8)((1-T) * From.B + T * To.B);
 Result.A = Cast(u8)((1-T) * From.A + T * To.A);
 
 return Result;
}

internal void
EquidistantPoints(f32 *Ti, u32 N)
{
 if (N > 1)
 {
  f32 TDelta = 1.0f / (N-1);
  f32 T = 0.0f;
  for (u32 I = 0; I < N; ++I)
  {
   Ti[I] = T;
   T += TDelta;
  }
 }
}

internal void
ChebyshevPoints(f32 *Ti, u32 N)
{
 for (u32 K = 0; K < N; ++K)
 {
  Ti[K] = CosF32(Pi32 * (2*K+1) / (2*N));
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
   f32 Num = PowOf2 * SinF32(Pi32 * (2*I+1) / (2*N));
   f32 Den = (N * SinF32(Pi32 * (2*I+1) * 0.5f));
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
CubicSplineEvaluate(f32 T, f32 *M, f32 *Ti, f32 *Y, u32 N)
{
 f32 Result = 0.0f;
 
 if (N == 0) {} // NOTE(hbr): Nothing to do
 else if (N == 1) { Result = Y[0]; } // NOTE(hbr): No interpolation in one point case
 else
 {
#if 0
  // NOTE(hbr): Binary search to find correct range.
  u32 I = 0;
  {
   // NOTE(hbr): Maybe optimize to not use binary search
   u32 Left = 0, Right = N-1;
   while (Left + 1 < Right)
   {
    u32 Middle = (Left + Right) >> 1;
    if (T >= Ti[Middle]) Left = Middle;
    else Right = Middle;
   }
   Assert(T >= Ti[Left]);
   Assert(Left == N-2 || T < Ti[Left+1]);
   I = Left;
  }
#else
  // NOTE(hbr): Just linearly search
  u32 I = 0;
  for (I = N-2; I > 0; --I)
  {
   if (T >= Ti[I]) break;
  }
#endif
  
  // NOTE(hbr): Maybe optimize Hi here and T-Ti
  f32 Term1 = M[I+1] / (6.0f * Hi(Ti, I)) * Cube(T - Ti[I]);
  f32 Term2 = M[I] / (6.0f * Hi(Ti, I)) * Cube(Ti[I+1] - T);
  f32 Term3 = (Y[I+1] / Hi(Ti, I) - M[I+1] / 6.0f * Hi(Ti, I)) * (T - Ti[I]);
  f32 Term4 = (Y[I] / Hi(Ti, I) - Hi(Ti, I) / 6.0f * M[I]) * (Ti[I+1] - T);
  Result = Term1 + Term2 + Term3 + Term4;
 }
 
 return Result;
}

#if 0
// NOTE(hbr): O(n^2) time, O(n) memory versions for reference
internal v2
BezierCurveEvaluate(f32 T, v2 *P, u32 N)
{
 temp_arena Temp = TempArena(0);
 
 v2 *E = PushArrayNonZero(Temp.Arena, N, v2);
 MemoryCopy(E, P, N * SizeOf(E[0]));
 
 for (u32 I = 1; I < N; ++I)
 {
  for (u32 J = 0; J < N-I; ++J)
  {
   E[J] = (1-T)*E[J] + T*E[J+1];
  }
 }
 
 v2 Result = E[0];
 EndTemp(Temp);
 
 return Result;
}

internal v2
BezierCurveEvaluateWeighted(f32 T, v2 *P, f32 *W, u32 N)
{
 temp_arena Temp = TempArena(0);
 
 v2 *EP = PushArrayNonZero(Temp.Arena, N, v2);
 f32 *EW = PushArrayNonZero(Temp.Arena, N, f32);
 MemoryCopy(EP, P, N * SizeOf(EP[0]));
 MemoryCopy(EW, W, N * SizeOf(EW[0]));
 
 for (u32 I = 1; I < N; ++I)
 {
  for (u32 J = 0; J < N-I; ++J)
  {
   f32 EWJ = (1-T)*EW[J] + T*EW[J+1];
   
   f32 WL = EW[J] / EWJ;
   f32 WR = EW[J+1] / EWJ;
   
   EP[J] = (1-T)*WL*EP[J] + T*WR*EP[J+1];
   EW[J] = EWJ;
  }
 }
 
 v2 Result = EP[0];
 EndTemp(Temp);
 
 return Result;
}
#else
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
BezierCurveEvaluateWeighted(f32 T, v2 *P, f32 *W, u32 N)
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
#endif

internal void
BezierCurveElevateDegree(v2 *P, u32 N)
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
BezierCurveElevateDegreeWeighted(v2 *P, f32 *W, u32 N)
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
internal bezier_lower_degree
BezierCurveLowerDegree(v2 *P, f32 *W, u32 N)
{
 bezier_lower_degree Result = {};
 
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
BezierCurveLowerDegree(v2 *P, f32 *W, u32 N)
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
 v2 Rotation = Rotation2DFromVector(SegmentEnd - SegmentBegin);
 v2 InvRotation = Rotation2DInverse(Rotation);
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
  f32 ProjectionLengthSquared = Square(DotUV) * Inv_SegmentLengthSquared;
  
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

internal rectangle2
EmptyAABB(void)
{
 rectangle2 Result = {};
 Result.Mini = V2(F32_INF, F32_INF);
 Result.Maxi = V2(-F32_INF, -F32_INF);
 
 return Result;
}

internal void
AddPointAABB(rectangle2 *AABB, v2 P)
{
 if (P.X < AABB->Mini.X) AABB->Mini.X = P.X;
 if (P.X > AABB->Maxi.X) AABB->Maxi.X = P.X;
 if (P.Y < AABB->Mini.Y) AABB->Mini.Y = P.Y;
 if (P.Y > AABB->Maxi.Y) AABB->Maxi.Y = P.Y;
}

internal b32
IsNonEmpty(rectangle2 *Rect)
{
 b32 Result = ((Rect->Mini.X <= Rect->Maxi.X) &&
               (Rect->Mini.Y <= Rect->Maxi.Y));
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
CameraTransform(v2 P, v2 Rotation, f32 Zoom)
{
 mat3_inv Result = {};
 
 v2 XAxis = Rotation;
 v2 YAxis = Rotate90DegreesAntiClockwise(Rotation);
 
 mat3 A = Rows3x3(XAxis, YAxis);
 A = Scale3x3(A, Zoom);
 v2 AP = -(A*P);
 A = Translate3x3(A, AP);
 Result.Forward = A;
 
 mat3 iA = Cols3x3(XAxis, YAxis);
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
ModelTransform(v2 P, v2 Rotation, v2 Scale)
{
 v2 XAxis = Rotation;
 v2 YAxis = Rotate90DegreesAntiClockwise(Rotation);
 
 mat3 A = Cols3x3(XAxis, YAxis);
 A = Scale3x3(A, Scale);
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