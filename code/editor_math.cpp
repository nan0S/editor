inline internal v2
V2(f32 X, f32 Y)
{
   v2 Result = {};
   Result.X = X;
   Result.Y = Y;
   
   return Result;
}

inline internal v2s
V2S32(s32 X, s32 Y)
{
   v2s Result = {};
   Result.X = X;
   Result.Y = Y;
   
   return Result;
}

inline internal color
MakeColor(u8 R, u8 G, u8 B, u8 A)
{
   color Result = {};
   Result.R = R;
   Result.G = G;
   Result.B = B;
   Result.A = A;
   
   return Result;
}

internal color
BrightenColor(color Color, f32 BrightenByRatio)
{
   color Result = {};
   Result.R = ClampTop(Cast(u32)((1.0f + BrightenByRatio) * Color.R), 255);
   Result.G = ClampTop(Cast(u32)((1.0f + BrightenByRatio) * Color.G), 255);
   Result.B = ClampTop(Cast(u32)((1.0f + BrightenByRatio) * Color.B), 255);
   Result.A = Color.A;
   
   return Result;
}

internal f32
Norm(v2 V)
{
   f32 Result = sqrtf(NormSquared(V));
   return Result;
}

internal f32
NormSquared(v2 V)
{
   f32 Result = V.X*V.X + V.Y*V.Y;
   return Result;
}

internal s32
NormSquared(v2s V)
{
   s32 Result = V.X*V.X + V.Y*V.Y;
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

internal rotation_2d
Rotation2D(f32 X, f32 Y)
{
   rotation_2d Rotation = {};
   Rotation.X = X;
   Rotation.Y = Y;
   
   return Rotation;
}

internal rotation_2d
Rotation2DZero(void)
{
   rotation_2d Result = Rotation2D(1.0f, 0.0f);
   return Result;
}

internal rotation_2d
Rotation2DFromVector(v2 Vector)
{
   Normalize(&Vector);
   rotation_2d Result = Rotation2D(Vector.X, Vector.Y);
   
   return Result;
}

internal rotation_2d
Rotation2DFromDegrees(f32 Degrees)
{
   f32 Radians = DegToRad32 * Degrees;
   rotation_2d Result = Rotation2DFromRadians(Radians);
   
   return Result;
}

internal rotation_2d
Rotation2DFromRadians(f32 Radians)
{
   rotation_2d Result = Rotation2D(cosf(Radians), sinf(Radians));
   return Result;
}

internal f32
Rotation2DToDegrees(rotation_2d Rotation)
{
   f32 Radians = Rotation2DToRadians(Rotation);
   f32 Degrees = RadToDeg32 * Radians;
   
   return Degrees;
}

internal f32
Rotation2DToRadians(rotation_2d Rotation)
{
   f32 Radians = Atan2F32(Rotation.Y, Rotation.X);
   return Radians;
}

internal rotation_2d
Rotation2DInverse(rotation_2d Rotation)
{
   rotation_2d InverseRotation = Rotation2D(Rotation.X, -Rotation.Y);
   return InverseRotation;
}

internal rotation_2d
Rotate90DegreesAntiClockwise(rotation_2d Rotation)
{
   rotation_2d Result = Rotation2D(-Rotation.Y, Rotation.X);
   return Result;
}

internal rotation_2d
Rotate90DegreesClockwise(rotation_2d Rotation)
{
   rotation_2d Result = Rotation2D(Rotation.Y, -Rotation.X);
   return Result;
}

internal rotation_2d
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
   
   rotation_2d Rotation = Rotation2D(Cos, Sin);
   return Rotation;
}

internal v2
RotateAround(v2 Point, v2 Center, rotation_2d Rotation)
{
   v2 Translated = Point - Center;
   v2 Rotated = V2(Rotation.X * Translated.X - Rotation.Y * Translated.Y,
                   Rotation.X * Translated.Y + Rotation.Y * Translated.X);
   v2 Result = Rotated + Center;
   
   return Result;
}

internal rotation_2d
CombineRotations2D(rotation_2d RotationA, rotation_2d RotationB)
{
   f32 RotationX = RotationA.X * RotationB.X - RotationA.Y * RotationB.Y;
   f32 RotationY = RotationA.X * RotationB.Y + RotationA.Y * RotationB.X;
   v2 RotationVector = V2(RotationX, RotationY);
   Normalize(&RotationVector);
   
   rotation_2d Result = Rotation2D(RotationVector.X, RotationVector.Y);
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
AngleSort(u64 NumPoints, v2 *Points)
{
   if (NumPoints > 0)
   {
      v2 BottomLeftMostPoint = Points[0];
      for (u64 PointIndex = 1;
           PointIndex < NumPoints;
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
      for (u64 Iteration = 0;
           Sorting;
           ++Iteration)
      {
         Sorting = false;
         for (u64 J = 1; J < NumPoints - Iteration; ++J)
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

internal u64
RemoveDupliatesSorted(u64 NumPoints, v2 *Points)
{
   u64 NumUnique = 0;
   if (NumPoints > 0)
   {
      NumUnique = 1;
      for (u64 PointIndex = 1;
           PointIndex < NumPoints;
           ++PointIndex)
      {
         if (Points[PointIndex] != Points[NumUnique-1])
         {
            Points[NumUnique++] = Points[PointIndex];
         }
      }
   }
   
   return NumUnique;
}

internal u64
CalcConvexHull(u64 PointCount, v2 *Points, v2 *OutPoints)
{
   u64 HullPointCount = 0;
   if (PointCount > 0)
   {
      MemoryCopy(OutPoints, Points, PointCount * SizeOf(OutPoints[0]));
      AngleSort(PointCount, OutPoints);
      u64 UniquePointCount = RemoveDupliatesSorted(PointCount, OutPoints);
      
      HullPointCount = 1;
      for (u64 PointIndex = 1; PointIndex < UniquePointCount; ++PointIndex)
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

internal line_vertices_allocation
LineVerticesAllocationNone(sf::Vertex *VerticesBuffer)
{
   line_vertices_allocation Result = {};
   Result.Type = LineVerticesAllocation_None;
   Result.VerticesBuffer = VerticesBuffer;
   
   return Result;
}

internal line_vertices_allocation
LineVerticesAllocationArena(arena *Arena)
{
   line_vertices_allocation Result = {};
   Result.Type = LineVerticesAllocation_Arena;
   Result.Arena = Arena;
   
   return Result;
}

// NOTE(hbr): Trinale-strip-based, no mitter, no-spiky line version.
// TODO(hbr): Loop logic is very ugly but works. Clean it up.
// Might have only work because we need only loop on convex hull order points.
internal line_vertices
CalculateLineVertices(u64 NumLinePoints, v2 *LinePoints,
                      f32 LineWidth, color LineColor, b32 Loop,
                      line_vertices_allocation Allocation)
{
   u64 N = NumLinePoints;
   if (Loop) N += 2;
   
   u64 MaxNumVertices = 0;
   if (N >= 2) MaxNumVertices = 2*2 + 4 * N;
   
   sf::Vertex *Vertices = 0;
   u64 CapVertices = 0;
   switch (Allocation.Type)
   {
      case LineVerticesAllocation_None: {
         Vertices = Allocation.VerticesBuffer;
      } break;
      
      case LineVerticesAllocation_Arena: {
         Vertices = PushArrayNonZero(Allocation.Arena, MaxNumVertices, sf::Vertex);
      } break;
   }
   
   u64 VertexIndex = 0;
   b32 IsLastInside = false;
   
   if (!Loop && NumLinePoints >= 2)
   {
      v2 A = LinePoints[0];
      v2 B = LinePoints[1];
      
      v2 V_Line = B - A;
      v2 NV_Line = Rotate90DegreesAntiClockwise(V_Line);
      Normalize(&NV_Line);
      
      Vertices[VertexIndex + 0].position = V2ToVector2f(A + 0.5f * LineWidth * NV_Line);
      Vertices[VertexIndex + 1].position = V2ToVector2f(A - 0.5f * LineWidth * NV_Line);
      
      Vertices[VertexIndex + 0].color = ColorToSFMLColor(LineColor);
      Vertices[VertexIndex + 1].color = ColorToSFMLColor(LineColor);
      
      VertexIndex += 2;
      
      IsLastInside = false;
   }
   
   for (u64 PointIndex = 1;
        PointIndex + 1 < N;
        ++PointIndex)
   {
      v2 A = LinePoints[PointIndex - 1];
      v2 B = LinePoints[(PointIndex + 0) % NumLinePoints];
      v2 C = LinePoints[(PointIndex + 1) % NumLinePoints];
      
      v2 V_Line = B - A;
      Normalize(&V_Line);
      v2 NV_Line = Rotate90DegreesAntiClockwise(V_Line);
      
      v2 V_Succ = C - B;
      Normalize(&V_Succ);
      v2 NV_Succ = Rotate90DegreesAntiClockwise(V_Succ);
      
      b32 LeftTurn = (Cross(V_Line, V_Succ) >= 0.0f);
      f32 TurnedHalfWidth = (LeftTurn ? 1.0f : -1.0f) * 0.5f * LineWidth;
      
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
      
      Vertices[VertexIndex + 0].color = ColorToSFMLColor(LineColor);
      Vertices[VertexIndex + 1].color = ColorToSFMLColor(LineColor);
      Vertices[VertexIndex + 2].color = ColorToSFMLColor(LineColor);
      
      if ((LeftTurn && IsLastInside) || (!LeftTurn && !IsLastInside))
      {
         Vertices[VertexIndex + 0].position = V2ToVector2f(B_Line);
         Vertices[VertexIndex + 1].position = V2ToVector2f(IntersectionPoint);
         Vertices[VertexIndex + 2].position = V2ToVector2f(B_Succ);
         
         VertexIndex += 3;
      }
      else
      {
         Vertices[VertexIndex + 0].position = V2ToVector2f(IntersectionPoint);
         Vertices[VertexIndex + 1].position = V2ToVector2f(B_Line);
         Vertices[VertexIndex + 2].position = V2ToVector2f(IntersectionPoint);
         Vertices[VertexIndex + 3].position = V2ToVector2f(B_Succ);
         
         Vertices[VertexIndex + 3].color = ColorToSFMLColor(LineColor);
         
         VertexIndex += 4;
      }
      
      IsLastInside = !LeftTurn;
   }
   
   if (!Loop)
   {
      if (NumLinePoints >= 2)
      {
         v2 A = LinePoints[N-2];
         v2 B = LinePoints[N-1];
         
         v2 V_Line = B - A;
         v2 NV_Line = Rotate90DegreesAntiClockwise(V_Line);
         Normalize(&NV_Line);
         
         v2 B_Inside  = B + 0.5f * LineWidth * NV_Line;
         v2 B_Outside = B - 0.5f * LineWidth * NV_Line;
         
         if (IsLastInside)
         {
            Vertices[VertexIndex + 0].position = V2ToVector2f(B_Outside);
            Vertices[VertexIndex + 1].position = V2ToVector2f(B_Inside);
         }
         else
         {
            Vertices[VertexIndex + 0].position = V2ToVector2f(B_Inside);
            Vertices[VertexIndex + 1].position = V2ToVector2f(B_Outside);
         }
         
         Vertices[VertexIndex + 0].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 1].color = ColorToSFMLColor(LineColor);
         
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
   
   line_vertices Result = {};
   Result.NumVertices = VertexIndex;
   Result.Vertices = Vertices;
   Result.CapVertices = CapVertices;
   Result.PrimitiveType = sf::TriangleStrip;
   
   return Result;
}

internal color
LerpColor(color From, color To, f32 T)
{
   color Result = {};
   Result.R = Cast(u8)((1-T) * From.R + T * To.R);
   Result.G = Cast(u8)((1-T) * From.G + T * To.G);
   Result.B = Cast(u8)((1-T) * From.B + T * To.B);
   Result.A = Cast(u8)((1-T) * From.A + T * To.A);
   
   return Result;
}

internal void
EquidistantPoints(f32 *Ti, u64 N)
{
   if (N > 1)
   {
      f32 TDelta = 1.0f / (N-1);
      f32 T = 0.0f;
      for (u64 I = 0; I < N; ++I)
      {
         Ti[I] = T;
         T += TDelta;
      }
   }
}

internal void
ChebyshevPoints(f32 *Ti, u64 N)
{
   for (u64 K = 0; K < N; ++K)
   {
      Ti[K] = cosf(Pi32 * (2*K+1) / (2*N));
   }
}

internal void
BarycentricOmega(f32 *Omega, f32 *Ti, u64 N)
{
   for (u64 I = 0; I < N; ++I)
   {
      f32 W = 1.0f;
      for (u64 J = 0; J < N; ++J)
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
BarycentricOmegaWerner(f32 *Omega, f32 *Ti, u64 N)
{
   temp_arena Temp = TempArena(0);
   
   f32 *Mem = PushArrayNonZero(Temp.Arena, N*N, f32);
   
   Mem[Idx(0, 0, N)] = 1.0f;
   for (u64 K = 1; K < N; ++K)
   {
      Mem[Idx(0, K, N)] = 0.0f;
   }
   
   for (u64 I = 1; I < N; ++I)
   {
      for (u64 K = 0; K <= N-I; ++K)
      {
         Mem[Idx(I, K, N)] = Mem[Idx(I-1, K, N)] / (Ti[K] - Ti[I]);
         Mem[Idx(K+1, I, N)] = Mem[Idx(K, I, N)] - Mem[Idx(I, K, N)];
      }
   }
   
   for (u64 I = 0; I < N; ++I)
   {
      Omega[I] = Mem[Idx(N-1, I, N)];
   }
   
   EndTemp(Temp);
}

internal void
BarycentricOmegaEquidistant(f32 *Omega, f32 *Ti, u64 N)
{
   if (N <= 1)
   {
      // NOTE(hbr): Do nothing
   }
   else
   {
      f32 W0 = 1.0f;
      for (u64 J = 1; J < N; ++J)
      {
         W0 /= (Ti[0] - Ti[J]);
      }
      
      Omega[0] = W0;
      
      f32 W = W0;
      for (u64 I = 0; I < N-1; ++I)
      {
         s64 Num = I - N;
         f32 Frac = Cast(f32)Num / (I + 1);
         W *= Frac;
         Omega[I+1] = W;
      }
   }
}

inline internal f32
PowerOf2(u64 K)
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
BarycentricOmegaChebychev(f32 *Omega, u64 N)
{
   if (N > 0)
   {
      f32 PowOf2 = PowerOf2(N-1);
      for (u64 I = 0; I < N; ++I)
      {
         f32 Num = PowOf2 * SinF32(Pi32 * (2*I+1) / (2*N));
         f32 Den = (N * SinF32(Pi32 * (2*I+1) * 0.5f));
         Omega[I] = Num / Den;
      }
   }
}

internal f32
BarycentricEvaluate(f32 T, f32 *Omega, f32 *Ti, f32 *Y, u64 N)
{
   for (u64 I = 0; I < N; ++I)
   {
      if (T == Ti[I])
      {
         return Y[I];
      }
   }
   
   f32 Num = 0.0f;
   f32 Den = 0.0f;
   for (u64 I = 0; I < N; ++I)
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
NewtonBeta(f32 *Beta, f32 *Ti, f32 *Y, u64 N)
{
   temp_arena Temp = TempArena(0);
   
   f32 *Mem = PushArrayNonZero(Temp.Arena, N*N, f32);
   for (u64 J = 0; J < N; ++J)
   {
      Mem[J] = Y[J];
   }
   
   for (u64 I = 1; I < N; ++I)
   {
      for (u64 J = 0; J < N-I; ++J)
      {
         Mem[I*N + J] = (Mem[(I-1)*N + J+1] - Mem[(I-1)*N + J]) / (Ti[J+I] - Ti[J]);
      }
   }
   
   for (u64 I = 0; I < N; ++I)
   {
      Beta[I] = Mem[I*N + 0];
   }
   
   EndTemp(Temp);
}

// NOTE(hbr): Memory optimized version
internal void
NewtonBetaFast(f32 *Beta, f32 *Ti, f32 *Y, u64 N)
{
   for (u64 J = 0; J < N; ++J)
   {
      Beta[J] = Y[J];
   }
   
   for (u64 I = 1; I < N; ++I)
   {
      f32 Save = Beta[I-1];
      for (u64 J = 0; J < N-I; ++J)
      {
         f32 Temp = Beta[I+J];
         Beta[I+J] = (Beta[I+J] - Save) / (Ti[I+J] - Ti[J]);
         Save = Temp;
      }
   }
}

internal f32
NewtonEvaluate(f32 T, f32 *Beta, f32 *Ti, u64 N)
{
   f32 Result = 0.0f;
   for (u64 I = N; I > 0; --I)
   {
      Result = Result * (T - Ti[I-1]) + Beta[I-1];
   }
   
   return Result;
}

// NOTE(hbr): Those should be local conveniance internal, but impossible in C.
inline internal f32 Hi(f32 *Ti, u64 I) { return Ti[I+1] - Ti[I]; }
inline internal f32 Bi(f32 *Ti, f32 *Y, u64 I) { return 1.0f / Hi(Ti, I) * (Y[I+1] - Y[I]); }
inline internal f32 Vi(f32 *Ti, u64 I) { return 2.0f * (Hi(Ti, I-1) + Hi(Ti, I)); }
inline internal f32 Ui(f32 *Ti, f32 *Y, u64 I) { return 6.0f * (Bi(Ti, Y, I) - Bi(Ti, Y, I-1)); }

// TODO(hbr): Temporary, remove
using ldb = f32;
template<typename T>
using vector = std::vector<T>;
const ldb eps = 0.000001f; // CUSTOM
int gauss(vector<vector<ldb>> a, vector<ldb> &ans) { // O(n^3)
   int n = Cast(int)a.size(), m = Cast(int)a[0].size()-1;
   vector<int> pos(m, -1);
   ldb det = 1; int rank = 0;
   for (int col = 0, row = 0; col < m && row < n; ++col) {
      int mx = row;
      for (int i = row; i < n; i++)
         if (abs(a[i][col]) > abs(a[mx][col]))
         mx = i;
      if (Abs(a[mx][col]) < eps) {
         det = 0;
         continue;
      }
      for (int i = col; i <= m; i++)
      {
         f32 Tmp = a[row][i];
         a[row][i] = a[mx][i];
         a[mx][i] = Tmp;
      }
      if (row != mx) det = -det;
      det *= a[row][col];
      pos[col] = row;
      for (int i = 0; i < n; i++) {
         if (i != row && Abs(a[i][col]) > eps) {
            ldb c = a[i][col] / a[row][col];
            for (int j = col; j <= m; j++)
               a[i][j] -= c*a[row][j];
         }
      }
      ++row; ++rank;
   }
   ans.assign(m, 0);
   for (int i = 0; i < m; i++)
      if(pos[i] != -1)
      ans[i] = a[pos[i]][m]/a[pos[i]][i];
   for (int i = 0; i < n; i++) {
      ldb sum = 0;
      for (int j = 0; j < m; j++) sum += ans[j] * a[i][j];
      if (Abs(sum - a[i][m]) > eps) return -1;
   }
   int free_vars = 0;
   for (int i = 0; i < m; i++)
      if (pos[i] == -1)
      ++free_vars;
   return free_vars;
}

internal void
CubicSplineNaturalM(f32 *M, f32 *Ti, f32 *Y, u64 N)
{
   temp_arena Temp = TempArena(0);
   
   if (N == 0) {} // NOTE(hbr): Nothing to calculate
   else if (N == 1) { M[0] = 0.0f; }
   else if (N == 2) { M[0] = M[N-1] = 0.0f; }
   else
   {
      M[0] = M[N-1] = 0.0f;
      
      f32 *Diag = PushArrayNonZero(Temp.Arena, N, f32);
      for (u64 I = 1; I < N-1; ++I)
      {
         Diag[I] = Vi(Ti, I);
         M[I] = Ui(Ti, Y, I);
      }
      
      for (u64 I = 1; I < N-2; ++I)
      {
         // NOTE(hbr): Maybe optimize Hi
         f32 Multiplier = Hi(Ti, I) / Diag[I];
         Diag[I+1] -= Multiplier * Hi(Ti, I);
         M[I+1] -= Multiplier * M[I];
      }
      
      for (u64 I = N-2; I > 1; --I)
      {
         f32 Multiplier = Hi(Ti, I-1) / Diag[I];
         M[I-1] -= Multiplier * M[I];
         M[I] /= Diag[I];
      }
      M[1] /= Diag[1];
   }
   
   EndTemp(Temp);
}

// NOTE(hbr): Those should be local conveniance internal, but impossible in C.
inline internal f32 Hi(f32 *Ti, u64 I, u64 N) { if (I == N-1) I -= N-1; return Ti[I+1] - Ti[I]; }
inline internal f32 Yi(f32 *Y, u64 I, u64 N) { if (I == N) I = 1; return Y[I]; }
inline internal f32 Bi(f32 *Ti, f32 *Y, u64 I, u64 N) { return 1.0f / Hi(Ti, I, N) * (Yi(Y, I+1, N) - Yi(Y, I, N)); }
inline internal f32 Vi(f32 *Ti, u64 I, u64 N) { return 2.0f * (Hi(Ti, I, N) + Hi(Ti, I+1, N)); }
inline internal f32 Ui(f32 *Ti, f32 *Y, u64 I, u64 N) { return 6.0f * (Bi(Ti, Y, I+1, N) - Bi(Ti, Y, I, N)); }

internal void
CubicSplinePeriodicM(f32 *M, f32 *Ti, f32 *Y, u64 N)
{
   if (N == 0) {} // NOTE(hbr): Nothing to calculate
   else if (N == 1) { M[0] = 0.0f; }
   else if (N == 2) { M[0] = M[N-1] = 0.0f; }
   else
   {
      Assert(Y[0] == Y[N-1]);
#if 0
      temp_arena Temp = TempArena(0);
      
      f32 *A = PushArrayNonZero(Temp.Arena, (N-1) * N, f32);
      for (u64 I = 0; I <= N-2; ++I)
      {
         A[Idx(I, I, N)] = Vi(Ti, I, N);
      }
      for (u64 I = 1; I <= N-2; ++I)
      {
         A[Idx(I-1, I, N)] = Hi(Ti, I, N);
      }
      for (u64 I = 1; I <= N-2; ++I)
      {
         A[Idx(I, I-1, N)] = Hi(Ti, I, N);
      }
      A[Idx(0, N-2, N)] = Hi(Ti, 0, N);
      A[Idx(N-2, 0, N)] = Hi(Ti, 0, N);
      for (u64 I = 0; I <= N-2; ++I)
      {
         A[Idx(I, N-1, N)] = Ui(Ti, Y, I, N);
      }
      
      GaussianElimination(A, N-1, M+1);
      M[0] = M[N-1];
      
      EndTemp(Temp);
#else
      vector<vector<ldb>> a(N-1, vector<ldb>(N));
      for (u64 I = 0; I <= N-2; ++I)
      {
         a[I][I] = Vi(Ti, I, N);
      }
      for (u64 I = 1; I <= N-2; ++I)
      {
         a[I-1][I] = Hi(Ti, I, N);
      }
      for (u64 I = 1; I <= N-2; ++I)
      {
         a[I][I-1] = Hi(Ti, I, N);
      }
      a[0][N-2] = Hi(Ti, 0, N);
      a[N-2][0] = Hi(Ti, 0, N);
      for (u64 I = 0; I <= N-2; ++I)
      {
         a[I][N-1] = Ui(Ti, Y, I, N);
      }
      
      vector<ldb> ans;
      gauss(a, ans);
      for (u64 I = 1; I < N; ++I)
      {
         M[I] = ans[I-1];
      }
      M[0] = M[N-1];
#endif
   }
}

internal f32
CubicSplineEvaluate(f32 T, f32 *M, f32 *Ti, f32 *Y, u64 N)
{
   f32 Result = 0.0f;
   
   if (N == 0) {} // NOTE(hbr): Nothing to do
   else if (N == 1) { Result = Y[0]; } // NOTE(hbr): No interpolation in one point case
   else
   {
#if 0
      // NOTE(hbr): Binary search to find correct range.
      u64 I = 0;
      {
         // NOTE(hbr): Maybe optimize to not use binary search
         u64 Left = 0, Right = N-1;
         while (Left + 1 < Right)
         {
            u64 Middle = (Left + Right) >> 1;
            if (T >= Ti[Middle]) Left = Middle;
            else Right = Middle;
         }
         Assert(T >= Ti[Left]);
         Assert(Left == N-2 || T < Ti[Left+1]);
         I = Left;
      }
#else
      // NOTE(hbr): Just linearly search
      u64 I = 0;
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
BezierCurveEvaluate(f32 T, v2 *P, u64 N)
{
   temp_arena Temp = TempArena(0);
   
   v2 *E = PushArrayNonZero(Temp.Arena, N, v2);
   MemoryCopy(E, P, N * SizeOf(E[0]));
   
   for (u64 I = 1; I < N; ++I)
   {
      for (u64 J = 0; J < N-I; ++J)
      {
         E[J] = (1-T)*E[J] + T*E[J+1];
      }
   }
   
   v2 Result = E[0];
   EndTemp(Temp);
   
   return Result;
}

internal v2
BezierCurveEvaluateWeighted(f32 T, v2 *P, f32 *W, u64 N)
{
   temp_arena Temp = TempArena(0);
   
   v2 *EP = PushArrayNonZero(Temp.Arena, N, v2);
   f32 *EW = PushArrayNonZero(Temp.Arena, N, f32);
   MemoryCopy(EP, P, N * SizeOf(EP[0]));
   MemoryCopy(EW, W, N * SizeOf(EW[0]));
   
   for (u64 I = 1; I < N; ++I)
   {
      for (u64 J = 0; J < N-I; ++J)
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
BezierCurveEvaluate(f32 T, v2 *P, u64 N)
{
   f32 H = 1.0f;
   f32 U = 1 - T;
   v2 Q = P[0];
   
   for (u64 K = 1; K < N; ++K)
   {
      H = H * T * (N - K);
      H = H / (K * U + H);
      Q = (1 - H) * Q + H * P[K];
   }
   
   return Q;
}

internal v2
BezierCurveEvaluateWeighted(f32 T, v2 *P, f32 *W, u64 N)
{
   f32 H = 1.0f;
   f32 U = 1 - T;
   v2 Q = P[0];
   
   for (u64 K = 1; K < N; ++K)
   {
      H = H * T * (N - K) * W[K];
      H = H / (K * U * W[K-1] + H);
      Q = (1 - H) * Q + H * P[K];
   }
   
   return Q;
}
#endif

internal void
BezierCurveElevateDegree(v2 *P, u64 N)
{
   if (N > 0)
   {
      v2 PN = P[N-1];
      f32 Inv_N = 1.0f / N;
      for (u64 I = N-1; I >= 1; --I)
      {
         f32 Mix= I * Inv_N;
         P[I] = Mix * P[I-1] + (1 - Mix) * P[I];
      }
      
      P[N] = PN;
   }
}

internal void
BezierCurveElevateDegreeWeighted(v2 *P, f32 *W, u64 N)
{
   if (N > 0)
   {
      f32 WN = W[N-1];
      v2 PN = P[N-1];
      f32 Inv_N = 1.0f / N;
      for (u64 I = N-1; I >= 1; --I)
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
BezierCurveLowerDegree(v2 *P, f32 *W, u64 N)
{
   bezier_lower_degree Result = {};
   
   if (N >= 2)
   {
      u64 H = ((N-1) >> 1) + 1;
      
      f32 Prev_Front_W = 0.0f;
      v2 Prev_Front_P = {};
      for (u64 K = 0; K < H; ++K)
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
      for (u64 K = N-1; K >= H; --K)
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
         Result.W_I = ClampBot(Prev_Front_W, EPS_F32);
         Result.W_II = ClampBot(Prev_Back_W, EPS_F32);
      }
   }
   
   return Result;
}
#else
// NOTE(hbr): Non-optimzed, O(N) memory version for reference
internal void
BezierCurveLowerDegree(v2 *P, f32 *W, u64 N)
{
   if (N >= 1)
   {
      temp_arena Temp = TempArena(0);
      defer { EndTemp(Temp); };
      
      f32 *Front_W = PushArrayNonZero(Temp.Arena, N-1, f32);
      f32 *Back_W = PushArrayNonZero(Temp.Arena, N-1, f32);
      
      {
         f32 Prev_Front_W = 0.0f;
         for (u64 K = 0; K < N-1; ++K)
         {
            f32 Alpha = Cast(f32)K / (N-1-K);
            Front_W[K] = (1 + Alpha) * W[K] - Alpha * Prev_Front_W;
            Prev_Front_W = Front_W[K];
         }
      }
      
      {   
         f32 Prev_Back_W = 0.0f;
         for (u64 K = N-1; K >= 1; --K)
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
         for (u64 K = 0; K < N-1; ++K)
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
         for (u64 K = N-1; K >= 1; --K)
         {
            f32 Alpha = Cast(f32)(N-1-K) / K;
            Back_P[K-1] = (1 + Alpha) * W[K]/Back_W[K-1] * P[K] - Alpha * Prev_Back_W/Back_W[K-1] * Prev_Back_P;
            Prev_Back_W = Back_W[K-1];
            Prev_Back_P = Back_P[K-1];
         }
      }
      
      u64 H = (N >> 1) + 1;
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
CalculateBezierCubicPointAt(u64 N, v2 *P, cubic_bezier_point *Out, u64 At)
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
   u64 i = At;
   
   Out[At].P0 = (N > 2 ? (w(i) - 1.0f/3.0f * dt * s(i)) : w(i));
   Out[At].P1 = w(i);
   Out[At].P2 = (N > 2 ? (w(i) + 1.0f/3.0f * dt * s(i)) : w(i));
   
#undef s
#undef s_
#undef c
#undef w
}

internal void
BezierCubicCalculateAllControlPoints(u64 N, v2 *P, cubic_bezier_point *Out)
{
   for (u64 I = 0; I < N; ++I)
   {
      CalculateBezierCubicPointAt(N, P, Out, I);
   }
}

internal void
BezierCurveSplit(f32 T, u64 N, v2 *P, f32 *W, 
                 v2 *LeftPoints, f32 *LeftWeights,
                 v2 *RightPoints, f32 *RightWeights)
{
   temp_arena Temp = TempArena(0);
   
   all_de_casteljau_intermediate_results Intermediate = DeCasteljauAlgorithm(Temp.Arena, T, P, W, N);
   
   {   
      u64 Index = 0;
      for (u64 I = 0; I < N; ++I)
      {
         LeftPoints[I] = Intermediate.P[Index];
         LeftWeights[I] = Intermediate.W[Index];
         Index += N - I;
      }
   }
   
   {
      u64 Index = Intermediate.TotalPointCount - 1;
      for (u64 I = 0; I < N; ++I)
      {
         RightPoints[I] = Intermediate.P[Index];
         RightWeights[I] = Intermediate.W[Index];
         Index -= I + 1;
      }
   }
   
   EndTemp(Temp);
}

internal all_de_casteljau_intermediate_results
DeCasteljauAlgorithm(arena *Arena, f32 T, v2 *P, f32 *W, u64 N)
{
   all_de_casteljau_intermediate_results Result = {};
   
   u64 TotalPointCount = N * (N+1) / 2;
   v2 *OutP = PushArrayNonZero(Arena, TotalPointCount, v2);
   f32   *OutW = PushArrayNonZero(Arena, TotalPointCount, f32);
   
   MemoryCopy(OutW, W, N * SizeOf(OutW[0]));
   MemoryCopy(OutP, P, N * SizeOf(OutP[0]));
   
   u64 Index = N;
   for (u64 Iteration = 1;
        Iteration < N;
        ++Iteration)
   {
      for (u64 J = 0; J < N-Iteration; ++J)
      {
         u64 IndexLeft = Index - (N-Iteration)-1;
         u64 IndexRight = Index - (N-Iteration);
         
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

internal void
GaussianElimination(f32 *A, u64 N, f32 *Solution)
{
   temp_arena Temp = TempArena(0);
   
   s64 *Pos = PushArrayNonZero(Temp.Arena, N, s64);
   MemorySet(Pos, -1, N * SizeOf(Pos[0]));
   
   for (u64 Col = 0, Row = 0;
        Col < N && Row < N;
        ++Col)
   {
      u64 Max = Row;
      for (u64 I = Row; I < N; ++I)
      {
         if (Abs(A[Idx(I, Col, N+1)]) > Abs(A[Idx(Max, Col, N+1)]))
         {
            Max = I;
         }
      }
      if (Abs(A[Idx(Max, Col, N+1)]) < EPS_F32)
      {
         continue;
      }
      for (u64 I = Col; I <= N; ++I)
      {
         // TODO(hbr): Make sure swap is ok here
         f32 X = A[Idx(Row, I, N+1)];
         A[Idx(Row, I, N+1)] = A[Idx(Max, I, N+1)];
         A[Idx(Max, I, N+1)] = X;
      }
      Pos[Col] = Row;
      for (u64 I = 0; I < N; ++I)
      {
         if (I != Row && Abs(A[Idx(I, Col, N+1)]) > EPS_F32)
         {
            f32 C = A[Idx(I, Col, N+1)] / A[Idx(Row, Col, N+1)];
            for (u64 J = Col; J <= N; ++J)
            {
               A[Idx(I, J, N+1)] -= C * A[Idx(Row, J, N+1)];
            }
         }
      }
      ++Row;
   }
   
   for (u64 I = 0; I < N; ++I)
   {
      if (Pos[I] != -1)
      {
         Assert(Pos[I] >= 0);
         u64 P = Pos[I];
         Solution[I] = A[Idx(P, N, N+1)] / A[Idx(P, I, N+1)];
      }
      else
      {
         Solution[I] = 0.0f;
      }
   }
   
   EndTemp(Temp);
}

internal b32
PointCollision(v2 Position, v2 Point, f32 PointRadius)
{
   v2 Delta = Position - Point;
   b32 Collision = ((Abs(Delta.X) <= PointRadius) && (Abs(Delta.Y) <= PointRadius));
   
   return Collision;
}

internal b32
SegmentCollision(v2 Position,
                 v2 LineA, v2 LineB, f32 LineWidth)
{
   b32 Result = false;
   
   // TODO(hbr): Remove, deprecated
#if 0
   
#if 0
   
   
   f32 BoundaryLeft   = Min(LineA.X, LineB.X) - 0.5f * LineWidth;
   f32 BoundaryRight  = Max(LineA.X, LineB.X) + 0.5f * LineWidth;
   f32 BoundaryBottom = Min(LineA.Y, LineB.Y) - 0.5f * LineWidth;
   f32 BoundaryTop    = Max(LineA.Y, LineB.Y) + 0.5f * LineWidth;
   
#else
   
   f32 BoundaryLeft   = Min(LineA.X, LineB.X);
   f32 BoundaryRight  = Max(LineA.X, LineB.X);
   f32 BoundaryBottom = Min(LineA.Y, LineB.Y);
   f32 BoundaryTop    = Max(LineA.Y, LineB.Y);
   
#endif
   
   if (BoundaryLeft <= Position.X && Position.X <= BoundaryRight &&
       BoundaryBottom <= Position.Y && Position.Y <= BoundaryTop)
   {
      f32 Distance = LineDistance(Position, LineA, LineB);
      Result = (Distance <= LineWidth * 0.5f);
   }
   
#else
   
   v2 U = LineA - LineB;
   v2 V = Position - LineB;
   
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
         
         f32 LineDist = Abs(A*Position.X + B*Position.Y + C) *
            SqrtF32(Inv_SegmentLengthSquared);
         
         Result = (LineDist <= 0.5f * LineWidth);
      }
   }
   
#endif
   
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