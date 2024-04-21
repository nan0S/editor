//- Vectors
inline function v2f32
V2F32(f32 X, f32 Y)
{
   v2f32 Result = {};
   Result.X = X;
   Result.Y = Y;
   
   return Result;
}

inline function v2s32
V2S32(s32 X, s32 Y)
{
   v2s32 Result = {};
   Result.X = X;
   Result.Y = Y;
   
   return Result;
}

inline function color
ColorMake(u8 R, u8 G, u8 B, u8 A)
{
   color Result = {};
   Result.R = R;
   Result.G = G;
   Result.B = B;
   Result.A = A;
   
   return Result;
}

//- Calculations, algebra
function f32
Norm(v2f32 V)
{
   f32 Result = sqrtf(NormSquared(V));
   return Result;
}

function f32
NormSquared(v2f32 V)
{
   f32 Result = V.X*V.X + V.Y*V.Y;
   return Result;
}

function s32
NormSquared(v2s32 V)
{
   s32 Result = V.X*V.X + V.Y*V.Y;
   return Result;
}

function void
Normalize(v2f32 *V)
{
   f32 NormValue = Norm(*V);
   if (NormValue != 0.0f)
   {
      *V = *V / NormValue;
   }
}

function f32
Dot(v2f32 U, v2f32 V)
{
   f32 Result = U.X*V.X + U.Y*V.Y;
   return Result;
}

function f32
Cross(v2f32 U, v2f32 V)
{
   f32 Result = U.X*V.Y - U.Y*V.X;
   return Result;
}

function rotation_2d
Rotation2D(f32 X, f32 Y)
{
   rotation_2d Rotation = {};
   Rotation.X = X;
   Rotation.Y = Y;
   
   return Rotation;
}

function rotation_2d
Rotation2DZero(void)
{
   rotation_2d Result = Rotation2D(1.0f, 0.0f);
   return Result;
}

function rotation_2d
Rotation2DFromVector(v2f32 Vector)
{
   Normalize(&Vector);
   rotation_2d Result = Rotation2D(Vector.X, Vector.Y);
   
   return Result;
}

function rotation_2d
Rotation2DFromDegrees(f32 Degrees)
{
   f32 Radians = DegToRad32 * Degrees;
   rotation_2d Result = Rotation2DFromRadians(Radians);
   
   return Result;
}

function rotation_2d
Rotation2DFromRadians(f32 Radians)
{
   rotation_2d Result = Rotation2D(cosf(Radians), sinf(Radians));
   return Result;
}

function f32
Rotation2DToDegrees(rotation_2d Rotation)
{
   f32 Radians = Rotation2DToRadians(Rotation);
   f32 Degrees = RadToDeg32 * Radians;
   
   return Degrees;
}

function f32
Rotation2DToRadians(rotation_2d Rotation)
{
   f32 Radians = atan2f(Rotation.Y, Rotation.X);
   return Radians;
}

function rotation_2d
Rotation2DInverse(rotation_2d Rotation)
{
   rotation_2d InverseRotation = Rotation2D(Rotation.X, -Rotation.Y);
   return InverseRotation;
}

function rotation_2d
Rotate90DegreesAntiClockwise(rotation_2d Rotation)
{
   rotation_2d Result = Rotation2D(-Rotation.Y, Rotation.X);
   return Result;
}

function rotation_2d
Rotate90DegreesClockwise(rotation_2d Rotation)
{
   rotation_2d Result = Rotation2D(Rotation.Y, -Rotation.X);
   return Result;
}

function rotation_2d
Rotation2DFromMovementAroundPoint(v2f32 From, v2f32 To, v2f32 Center)
{
   v2f32 FromTranslated = From - Center;
   v2f32 ToTranslated = To - Center;
   
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

function v2f32
RotateAround(v2f32 Point, v2f32 Center, rotation_2d Rotation)
{
   v2f32 Translated = Point - Center;
   v2f32 Rotated = V2F32(Rotation.X * Translated.X - Rotation.Y * Translated.Y,
                         Rotation.X * Translated.Y + Rotation.Y * Translated.X);
   v2f32 Result = Rotated + Center;
   
   return Result;
}

function rotation_2d
CombineRotations2D(rotation_2d RotationA, rotation_2d RotationB)
{
   f32 RotationX = RotationA.X * RotationB.X - RotationA.Y * RotationB.Y;
   f32 RotationY = RotationA.X * RotationB.Y + RotationA.Y * RotationB.X;
   v2f32 RotationVector = V2F32(RotationX, RotationY);
   Normalize(&RotationVector);
   
   rotation_2d Result = Rotation2D(RotationVector.X, RotationVector.Y);
   return Result;
}

internal b32
AngleCompareLess(v2f32 A, v2f32 B, v2f32 O)
{
   v2f32 U = A - O;
   v2f32 V = B - O;
   
   f32 C = Cross(U, V);
   f32 LU = Dot(U, U);
   f32 LV = Dot(V, V);
   
   b32 Result = (C > 0.0f || (C == 0.0f && LU < LV));
   return Result;
}

internal void
AngleSort(u64 NumPoints, v2f32 *Points)
{
   if (NumPoints > 0)
   {
      v2f32 BottomLeftMostPoint = Points[0];
      for (u64 PointIndex = 1;
           PointIndex < NumPoints;
           ++PointIndex)
      {
         v2f32 Point = Points[PointIndex];
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
            v2f32 A = Points[J-1];
            v2f32 B = Points[J];
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
RemoveDupliatesSorted(u64 NumPoints, v2f32 *Points)
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

function num_convex_hull_points
CalculateConvexHull(u64 NumPoints, v2f32 *Points,
                    v2f32 *OutputConvexHullPoints)
{
   num_convex_hull_points NumConvexHullPoints = 0;
   
   if (NumPoints > 0)
   {
      MemoryCopy(OutputConvexHullPoints, Points, NumPoints * SizeOf(OutputConvexHullPoints[0]));
      AngleSort(NumPoints, OutputConvexHullPoints);
      u64 NumUniquePoints = RemoveDupliatesSorted(NumPoints, OutputConvexHullPoints);
      
      NumConvexHullPoints = 1;
      for (u64 PointIndex = 1;
           PointIndex < NumUniquePoints;
           ++PointIndex)
      {
         v2f32 Point = OutputConvexHullPoints[PointIndex];
         while (NumConvexHullPoints >= 2)
         {
            v2f32 O = OutputConvexHullPoints[NumConvexHullPoints - 2];
            v2f32 U = OutputConvexHullPoints[NumConvexHullPoints - 1] - O;
            v2f32 V = Point - O;
            
            if (Cross(U, V) <= 0.0f)
            {
               --NumConvexHullPoints;
            }
            else
            {
               break;
            }
         }
         
         OutputConvexHullPoints[NumConvexHullPoints++] = Point;
      }
   }
   
   return NumConvexHullPoints;
}

function line_vertices_allocation
LineVerticesAllocationNone(sf::Vertex *VerticesBuffer)
{
   line_vertices_allocation Result = {};
   Result.Type = LineVerticesAllocation_None;
   Result.VerticesBuffer = VerticesBuffer;
   
   return Result;
}

function line_vertices_allocation
LineVerticesAllocationHeap(line_vertices OldVertices)
{
   line_vertices_allocation Result = {};
   Result.Type = LineVerticesAllocation_Heap;
   Result.OldVertices = OldVertices;
   
   return Result;
}

function line_vertices_allocation
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
function line_vertices
CalculateLineVertices(u64 NumLinePoints, v2f32 *LinePoints,
                      f32 LineWidth, color LineColor, b32 Loop,
                      line_vertices_allocation Allocation)
{
   u64 N = NumLinePoints;
   if (Loop) N += 2;
   
   u64 MaximumNumVertices = 0;
   if (N >= 2) MaximumNumVertices = 2*2 + 4 * N;
   
   sf::Vertex *Vertices = 0;
   u64 CapVertices = 0;
   switch (Allocation.Type)
   {
      case LineVerticesAllocation_None: {
         Vertices = Allocation.VerticesBuffer;
      } break;
      
      case LineVerticesAllocation_Heap: {
         Vertices = Allocation.OldVertices.Vertices;
         CapVertices = Allocation.OldVertices.CapVertices;
         ArrayReserve(Vertices, CapVertices, MaximumNumVertices);
      } break;
      
      case LineVerticesAllocation_Arena: {
         Vertices = PushArray(Allocation.Arena, MaximumNumVertices, sf::Vertex);
      } break;
   }
   
   u64 VertexIndex = 0;
   b32 IsLastInside = false;
   
   if (!Loop && NumLinePoints >= 2)
   {
      v2f32 A = LinePoints[0];
      v2f32 B = LinePoints[1];
      
      v2f32 V_Line = B - A;
      v2f32 NV_Line = Rotate90DegreesAntiClockwise(V_Line);
      Normalize(&NV_Line);
      
      Vertices[VertexIndex + 0].position = V2F32ToVector2f(A + 0.5f * LineWidth * NV_Line);
      Vertices[VertexIndex + 1].position = V2F32ToVector2f(A - 0.5f * LineWidth * NV_Line);
      
      Vertices[VertexIndex + 0].color = ColorToSFMLColor(LineColor);
      Vertices[VertexIndex + 1].color = ColorToSFMLColor(LineColor);
      
      VertexIndex += 2;
      
      IsLastInside = false;
   }
   
   for (u64 PointIndex = 1;
        PointIndex + 1 < N;
        ++PointIndex)
   {
      v2f32 A = LinePoints[PointIndex - 1];
      v2f32 B = LinePoints[(PointIndex + 0) % NumLinePoints];
      v2f32 C = LinePoints[(PointIndex + 1) % NumLinePoints];
      
      v2f32 V_Line = B - A;
      Normalize(&V_Line);
      v2f32 NV_Line = Rotate90DegreesAntiClockwise(V_Line);
      
      v2f32 V_Succ = C - B;
      Normalize(&V_Succ);
      v2f32 NV_Succ = Rotate90DegreesAntiClockwise(V_Succ);
      
      b32 LeftTurn = (Cross(V_Line, V_Succ) >= 0.0f);
      f32 TurnedHalfWidth = (LeftTurn ? 1.0f : -1.0f) * 0.5f * LineWidth;
      
      line_intersection Intersection = LineIntersection(A + TurnedHalfWidth * NV_Line,
                                                        B + TurnedHalfWidth * NV_Line,
                                                        B + TurnedHalfWidth * NV_Succ,
                                                        C + TurnedHalfWidth * NV_Succ);
      
      v2f32 IntersectionPoint = {};
      if (Intersection.IsOneIntersection)
      {
         IntersectionPoint = Intersection.IntersectionPoint;
      }
      else
      {
         IntersectionPoint = B + TurnedHalfWidth * NV_Line;
      }
      
      v2f32 B_Line = B - TurnedHalfWidth * NV_Line;
      v2f32 B_Succ = B - TurnedHalfWidth * NV_Succ;
      
      Vertices[VertexIndex + 0].color = ColorToSFMLColor(LineColor);
      Vertices[VertexIndex + 1].color = ColorToSFMLColor(LineColor);
      Vertices[VertexIndex + 2].color = ColorToSFMLColor(LineColor);
      
      if ((LeftTurn && IsLastInside) || (!LeftTurn && !IsLastInside))
      {
         Vertices[VertexIndex + 0].position = V2F32ToVector2f(B_Line);
         Vertices[VertexIndex + 1].position = V2F32ToVector2f(IntersectionPoint);
         Vertices[VertexIndex + 2].position = V2F32ToVector2f(B_Succ);
         
         VertexIndex += 3;
      }
      else
      {
         Vertices[VertexIndex + 0].position = V2F32ToVector2f(IntersectionPoint);
         Vertices[VertexIndex + 1].position = V2F32ToVector2f(B_Line);
         Vertices[VertexIndex + 2].position = V2F32ToVector2f(IntersectionPoint);
         Vertices[VertexIndex + 3].position = V2F32ToVector2f(B_Succ);
         
         Vertices[VertexIndex + 3].color = ColorToSFMLColor(LineColor);
         
         VertexIndex += 4;
      }
      
      IsLastInside = !LeftTurn;
   }
   
   if (!Loop)
   {
      if (NumLinePoints >= 2)
      {
         v2f32 A = LinePoints[N-2];
         v2f32 B = LinePoints[N-1];
         
         v2f32 V_Line = B - A;
         v2f32 NV_Line = Rotate90DegreesAntiClockwise(V_Line);
         Normalize(&NV_Line);
         
         v2f32 B_Inside  = B + 0.5f * LineWidth * NV_Line;
         v2f32 B_Outside = B - 0.5f * LineWidth * NV_Line;
         
         if (IsLastInside)
         {
            Vertices[VertexIndex + 0].position = V2F32ToVector2f(B_Outside);
            Vertices[VertexIndex + 1].position = V2F32ToVector2f(B_Inside);
         }
         else
         {
            Vertices[VertexIndex + 0].position = V2F32ToVector2f(B_Inside);
            Vertices[VertexIndex + 1].position = V2F32ToVector2f(B_Outside);
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

function f32
LerpF32(f32 From, f32 To, f32 T)
{
   return (1-T) * From + T * To;
}

function v2f32
LerpV2F32(v2f32 From, v2f32 To, f32 T)
{
   return (1-T) * From + T * To;
}

function color
LerpColor(color From, color To, f32 T)
{
   color Result = {};
   Result.R = Cast(u8)((1-T) * From.R + T * To.R);
   Result.G = Cast(u8)((1-T) * From.G + T * To.G);
   Result.B = Cast(u8)((1-T) * From.B + T * To.B);
   Result.A = Cast(u8)((1-T) * From.A + T * To.A);
   
   return Result;
}

//- Interpolation
function void
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

function void
ChebyshevPoints(f32 *Ti, u64 N)
{
   for (u64 K = 0; K < N; ++K)
   {
      Ti[K] = cosf(Pi32 * (2*K+1) / (2*N));
   }
}

function void
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
function void
BarycentricOmegaWerner(f32 *Omega, f32 *Ti, u64 N)
{
   auto Scratch = ScratchArena(0);
   
   f32 *Mem = PushArray(Scratch.Arena, N*N, f32);
   
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
   
   ReleaseScratch(Scratch);
}

function void
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

function void
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

function f32
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
function void
NewtonBeta(f32 *Beta, f32 *Ti, f32 *Y, u64 N)
{
   auto Scratch = ScratchArena(0);
   
   f32 *Mem = PushArray(Scratch.Arena, N*N, f32);
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
   
   ReleaseScratch(Scratch);
}

// NOTE(hbr): Memory optimized version
function void
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

function f32
NewtonEvaluate(f32 T, f32 *Beta, f32 *Ti, u64 N)
{
   f32 Result = 0.0f;
   for (u64 I = N; I > 0; --I)
   {
      Result = Result * (T - Ti[I-1]) + Beta[I-1];
   }
   
   return Result;
}

// NOTE(hbr): Those should be local conveniance function, but impossible in C.
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

function void
CubicSplineNaturalM(f32 *M, f32 *Ti, f32 *Y, u64 N)
{
   auto Scratch = ScratchArena(0);
   
   if (N == 0) {} // NOTE(hbr): Nothing to calculate
   else if (N == 1) { M[0] = 0.0f; }
   else if (N == 2) { M[0] = M[N-1] = 0.0f; }
   else
   {
      M[0] = M[N-1] = 0.0f;
      
      f32 *Diag = PushArray(Scratch.Arena, N, f32);
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
   
   ReleaseScratch(Scratch);
}

// NOTE(hbr): Those should be local conveniance function, but impossible in C.
inline internal f32 Hi(f32 *Ti, u64 I, u64 N) { if (I == N-1) I -= N-1; return Ti[I+1] - Ti[I]; }
inline internal f32 Yi(f32 *Y, u64 I, u64 N) { if (I == N) I = 1; return Y[I]; }
inline internal f32 Bi(f32 *Ti, f32 *Y, u64 I, u64 N) { return 1.0f / Hi(Ti, I, N) * (Yi(Y, I+1, N) - Yi(Y, I, N)); }
inline internal f32 Vi(f32 *Ti, u64 I, u64 N) { return 2.0f * (Hi(Ti, I, N) + Hi(Ti, I+1, N)); }
inline internal f32 Ui(f32 *Ti, f32 *Y, u64 I, u64 N) { return 6.0f * (Bi(Ti, Y, I+1, N) - Bi(Ti, Y, I, N)); }

function void
CubicSplinePeriodicM(f32 *M, f32 *Ti, f32 *Y, u64 N)
{
   if (N == 0) {} // NOTE(hbr): Nothing to calculate
   else if (N == 1) { M[0] = 0.0f; }
   else if (N == 2) { M[0] = M[N-1] = 0.0f; }
   else
   {
      Assert(Y[0] == Y[N-1]);
#if 0
      auto Scratch = ScratchArena(0);
      
      f32 *A = PushArray(Scratch.Arena, (N-1) * N, f32);
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
      
      ReleaseScratch(Scratch);
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

function f32
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

function v2f32
BezierCurveEvaluate(f32 T, v2f32 *P, u64 N)
{
   auto Scratch = ScratchArena(0);
   defer { ReleaseScratch(Scratch); };
   
   v2f32 *E = PushArray(Scratch.Arena, N, v2f32);
   MemoryCopy(E, P, N * SizeOf(E[0]));
   
   for (u64 I = 1; I < N; ++I)
   {
      for (u64 J = 0; J < N-I; ++J)
      {
         E[J] = (1-T)*E[J] + T*E[J+1];
      }
   }
   
   v2f32 Result = E[0];
   return Result;
}

function v2f32
BezierWeightedCurveEvaluate(f32 T, v2f32 *P, f32 *W, u64 N)
{
   auto Scratch = ScratchArena(0);
   defer { ReleaseScratch(Scratch); };
   
   v2f32 *EP = PushArray(Scratch.Arena, N, v2f32);
   f32 *EW = PushArray(Scratch.Arena, N, f32);
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
   
   v2f32 Result = EP[0];
   return Result;
}

function v2f32
BezierCurveEvaluateFast(f32 T, v2f32 *P, u64 N)
{
   f32 H = 1.0f;
   f32 U = 1 - T;
   v2f32 Q = P[0];
   
   for (u64 K = 1; K < N; ++K)
   {
      H = H * T * (N - K);
      H = H / (K * U + H);
      Q = (1 - H) * Q + H * P[K];
   }
   
   return Q;
}

function v2f32
BezierWeightedCurveEvaluateFast(f32 T, v2f32 *P, f32 *W, u64 N)
{
   f32 H = 1.0f;
   f32 U = 1 - T;
   v2f32 Q = P[0];
   
   for (u64 K = 1; K < N; ++K)
   {
      H = H * T * (N - K) * W[K];
      H = H / (K * U * W[K-1] + H);
      Q = (1 - H) * Q + H * P[K];
   }
   
   return Q;
}

function void
BezierCurveElevateDegree(v2f32 *P, u64 N)
{
   if (N >= 1)
   {
      v2f32 PN = P[N-1];
      f32 Inv_N = 1.0f / N;
      for (u64 I = N-1; I >= 1; --I)
      {
         f32 Mix= I * Inv_N;
         P[I] = Mix * P[I-1] + (1 - Mix) * P[I];
      }
      
      P[N] = PN;
   }
}

function void
BezierWeightedCurveElevateDegree(v2f32 *P, f32 *W, u64 N)
{
   if (N >= 1)
   {
      f32 WN = W[N-1];
      v2f32 PN = P[N-1];
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

function bezier_lower_degree
BezierCurveLowerDegree(v2f32 *P, u64 N)
{
   bezier_lower_degree Result = {};
   
   if (N >= 2)
   {
      u64 H = ((N-1) >> 1) + 1;
      
      v2f32 Prev_Front_P = {};
      for (u64 K = 0; K < H; ++K)
      {
         f32 Alpha = Cast(f32)K / (N-1-K);
         v2f32 New_P = (1 + Alpha) * P[K] - Alpha * Prev_Front_P;
         P[K] = New_P;
         Prev_Front_P = New_P;
      }
      // NOTE(hbr): Prev_Front_P == P_I[H-1] at this point
      
      v2f32 Prev_Back_P = P[N-1];
      v2f32 Save_P = P[N-1];
      for (u64 K = N-1; K >= H; --K)
      {
         f32 Alpha = Cast(f32)(N-1) / K;
         v2f32 New_P = Alpha * Save_P + (1 - Alpha) * Prev_Back_P;
         Save_P = P[K-1];
         P[K-1] = New_P;
         Prev_Back_P = New_P;
      }
      // NOTE(hbr): Prev_Back_P == P_II[H-1] at this point
      
      if (!ApproxEq32(Prev_Front_P.X, Prev_Back_P.X) ||
          !ApproxEq32(Prev_Front_P.Y, Prev_Back_P.Y))
      {
         Result.Failure = true;
         Result.MiddlePointIndex = H-1;
         Result.P_I = Prev_Front_P;
         Result.P_II = Prev_Back_P;
      }
   }
   
   return Result;
}

#if 1

// NOTE(hbr): Optimized, O(1) memory version
function bezier_lower_degree
BezierWeightedCurveLowerDegree(v2f32 *P, f32 *W, u64 N)
{
   bezier_lower_degree Result = {};
   
   if (N >= 2)
   {
      u64 H = ((N-1) >> 1) + 1;
      
      f32 Prev_Front_W = 0.0f;
      v2f32 Prev_Front_P = {};
      for (u64 K = 0; K < H; ++K)
      {
         f32 Alpha = Cast(f32)K / (N-1-K);
         
         f32 Left_W = (1 + Alpha) * W[K];
         f32 Right_W = Alpha * Prev_Front_W;
         f32 New_W = Left_W - Right_W;
         
         f32 Inv_New_W = 1.0f / New_W;
         v2f32 New_P = Left_W * Inv_New_W * P[K] - Right_W * Inv_New_W * Prev_Front_P;
         
         W[K] = New_W;
         P[K] = New_P;
         
         Prev_Front_W = New_W;
         Prev_Front_P = New_P;
      }
      // NOTE(hbr): Prev_Front_W == W_I[H-1] and Prev_Front_P == P_I[H-1]
      // at this point
      
      f32 Prev_Back_W = 0.0f;
      v2f32 Prev_Back_P = {};
      f32 Save_W = W[N-1];
      v2f32 Save_P = P[N-1];
      for (u64 K = N-1; K >= H; --K)
      {
         f32 Alpha = Cast(f32)(N-1) / K;
         
         f32 Left_W = Alpha * Save_W;
         f32 Right_W = (1 - Alpha) * Prev_Back_W;
         f32 New_W = Left_W + Right_W;
         
         f32 Inv_New_W = 1.0f / New_W;
         v2f32 New_P = Left_W * Inv_New_W * Save_P + Right_W * Inv_New_W * Prev_Back_P;
         
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
         Result.W_I = ClampBot(Prev_Front_W, Epsilon32);
         Result.W_II = ClampBot(Prev_Back_W, Epsilon32);
      }
   }
   
   return Result;
}

#else

// NOTE(hbr): Non-optimzed, O(N) memory version for reference
function void
BezierWeightedCurveLowerDegree(v2f32 *P, f32 *W, u64 N)
{
   if (N >= 1)
   {
      auto Scratch = ScratchArena(0);
      defer { ReleaseScratch(Scratch); };
      
      f32 *Front_W = PushArray(Scratch.Arena, N-1, f32);
      f32 *Back_W = PushArray(Scratch.Arena, N-1, f32);
      
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
      
      v2f32 *Front_P = PushArray(Scratch.Arena, N-1, v2f32);
      v2f32 *Back_P = PushArray(Scratch.Arena, N-1, v2f32);
      
      {   
         f32 Prev_Front_W = 0.0f;
         v2f32 Prev_Front_P = {};
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
         v2f32 Prev_Back_P = {};
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

// TODO(hbr): Refactor
function void
BezierCubicCalculateAllControlPoints(v2f32 *P, u64 N, v2f32 *Output)
{
   if (N > 0)
   {
      auto Scratch = ScratchArena(0);
      defer { ReleaseScratch(Scratch); };
      
      // NOTE(hbr): Assume points are equidistant
      f32 DeltaT = 1.0f / (N - 1);
      f32 Inv_DeltaT = 1.0f / DeltaT;
      f32 A = 0.5f;
      f32 OneThird = 1.0f / 3.0f;
      f32 OneThirdDeltaT = OneThird * DeltaT;
      
      v2f32 *S = PushArray(Scratch.Arena, N, v2f32);
      for (u64 I = 1; I + 1 < N; ++I)
      {
         // TODO(hbr): Optiomize P lookups
         v2f32 C_I_1 = Inv_DeltaT * (P[I] - P[I-1]);
         v2f32 C_I = Inv_DeltaT * (P[I+1] - P[I]);
         // TODO(hbr): Optimize 1-A
         S[I] = (1.0f - A) * C_I_1 + A * C_I;
      }
      
      if (N >= 2)
      {
         v2f32 C_0 = Inv_DeltaT * (P[1] - P[0]);
         v2f32 C_N_2 = Inv_DeltaT * (P[N-1] - P[N-2]);
         S[0] = 2.0f * C_0 - S[1];
         S[N-1] = 2.0f * C_N_2 - S[N-2];
      }
      else
      {
         Assert(N == 1);
         S[0] = V2F32(1.0f, 1.0f);
      }
      
      u64 OutputIndex = 0;
      Output[OutputIndex++] = P[0];
      for (u64 I = 1; I < N; ++I)
      {
         v2f32 W_I = P[I];
         v2f32 W_I_1 = P[I-1];
         
         v2f32 W_I_2_3 = W_I_1 + OneThirdDeltaT * S[I-1];
         v2f32 W_I_1_3 = W_I - OneThirdDeltaT * S[I];
         
         Output[OutputIndex + 0] = W_I_2_3;
         Output[OutputIndex + 1] = W_I_1_3;
         Output[OutputIndex + 2] = W_I;
         
         OutputIndex += 3;
      }
   }
   
   // TODO(hbr): Refactor
   if (N > 0)
   {
      if (N == 1)
      {
         Output[-1] = Output[0] - V2F32(0.1f, 0.0f);
         Output[ 1] = Output[0] + V2F32(0.1f, 0.0f);
      }
      else
      {
         Output[-1] = Output[0] -  (Output[1] - Output[0]);
         Output[3 * N - 2] = Output[3 * N - 3] - (Output[3 * N - 4] - Output[3 * N - 3]);
      }
   }
}

function void
BezierCurveSplit(f32 T, v2f32 *P, f32 *W, u64 N,
                 v2f32 *LeftControlPoints, f32 *LeftControlPointWeights,
                 v2f32 *RightControlPoints, f32 *RightControlPointWeights)
{
   auto Scratch = ScratchArena(0);
   defer { ReleaseScratch(Scratch); };
   
   f32 *We = PushArray(Scratch.Arena, N * N, f32);
   v2f32 *Pe = PushArray(Scratch.Arena, N * N, v2f32);
   
   DeCasteljauAlgorithm(T, P, W, N, Pe, We);
   
   for (u64 I = 0; I < N; ++I)
   {
      u64 Index = Idx(I, 0, N);
      LeftControlPointWeights[I] = We[Index];
      LeftControlPoints[I] = Pe[Index];
   }
   
   for (u64 I = 0; I < N; ++I)
   {
      u64 Index = Idx(N-1-I, I, N);
      RightControlPointWeights[I] = We[Index];
      RightControlPoints[I] = Pe[Index];
   }
}

function void
DeCasteljauAlgorithm(f32 T, v2f32 *P, f32 *W, u64 N,
                     v2f32 *OutputP, f32 *OutputW)
{
   MemoryCopy(OutputW, W, N * SizeOf(OutputW[0]));
   MemoryCopy(OutputP, P, N * SizeOf(OutputP[0]));
   
   for (u64 Iteration = 1;
        Iteration < N;
        ++Iteration)
   {
      for (u64 J = 0; J < N-Iteration; ++J)
      {
         u64 IndexLeft = Idx(Iteration-1, J, N);
         u64 IndexRight = Idx(Iteration-1, J+1, N);
         u64 Index = Idx(Iteration, J, N);
         
         f32 WeightLeft = (1-T) * OutputW[IndexLeft];
         f32 WeightRight = T * OutputW[IndexRight];
         f32 Weight = WeightLeft + WeightRight;
         f32 InvWeight = 1.0f / Weight;
         
         OutputW[Index] = Weight;
         OutputP[Index] = WeightLeft * InvWeight * OutputP[IndexLeft] +
            WeightRight * InvWeight * OutputP[IndexRight];
      }
   }
}

function void
GaussianElimination(f32 *A, u64 N, f32 *Solution)
{
   auto Scratch = ScratchArena(0);
   
   s64 *Pos = PushArray(Scratch.Arena, N, s64);
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
      if (Abs(A[Idx(Max, Col, N+1)]) < Epsilon32)
      {
         continue;
      }
      for (u64 I = Col; I <= N; ++I)
      {
         f32 Temp = A[Idx(Row, I, N+1)];
         A[Idx(Row, I, N+1)] = A[Idx(Max, I, N+1)];
         A[Idx(Max, I, N+1)] = Temp;
      }
      Pos[Col] = Row;
      for (u64 I = 0; I < N; ++I)
      {
         if (I != Row && Abs(A[Idx(I, Col, N+1)]) > Epsilon32)
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
   
   ReleaseScratch(Scratch);
}

//- Collisions, intersections, geometry
function b32
PointCollision(v2f32 Position, v2f32 Point, f32 PointRadius)
{
   v2f32 Delta = Position - Point;
   b32 Collision = ((Abs(Delta.X) <= PointRadius) && (Abs(Delta.Y) <= PointRadius));
   
   return Collision;
}

function b32
SegmentCollision(v2f32 Position,
                 v2f32 LineA, v2f32 LineB, f32 LineWidth)
{
   b32 Result = false;
   
   // TODO(hbr): Remove, deprecated
#if 0
   
#if 0
   
   
   f32 BoundaryLeft   = Minimum(LineA.X, LineB.X) - 0.5f * LineWidth;
   f32 BoundaryRight  = Maximum(LineA.X, LineB.X) + 0.5f * LineWidth;
   f32 BoundaryBottom = Minimum(LineA.Y, LineB.Y) - 0.5f * LineWidth;
   f32 BoundaryTop    = Maximum(LineA.Y, LineB.Y) + 0.5f * LineWidth;
   
#else
   
   f32 BoundaryLeft   = Minimum(LineA.X, LineB.X);
   f32 BoundaryRight  = Maximum(LineA.X, LineB.X);
   f32 BoundaryBottom = Minimum(LineA.Y, LineB.Y);
   f32 BoundaryTop    = Maximum(LineA.Y, LineB.Y);
   
#endif
   
   if (BoundaryLeft <= Position.X && Position.X <= BoundaryRight &&
       BoundaryBottom <= Position.Y && Position.Y <= BoundaryTop)
   {
      f32 Distance = LineDistance(Position, LineA, LineB);
      Result = (Distance <= LineWidth * 0.5f);
   }
   
#else
   
   v2f32 U = LineA - LineB;
   v2f32 V = Position - LineB;
   
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

function line_intersection
LineIntersection(v2f32 A, v2f32 B, v2f32 C, v2f32 D)
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
      Result.IntersectionPoint = V2F32(X/Det, Y/Det);
   }
   
   return Result;
}