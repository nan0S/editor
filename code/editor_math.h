#ifndef EDITOR_MATH_H
#define EDITOR_MATH_H

#define Pi32 3.14159265359f
#define DegToRad32 (Pi32 / 180.0f)
#define RadToDeg32 (180.0f / Pi32)

#define PowF32(Base, Exponent) powf((Base), (Exponent))
#define SinF32(X)              sinf((X))
#define Log10F32(X)            log10f(X)
#define FloorF32(X)            floorf(X)
#define CeilF32(X)             ceilf(X)
#define RoundF32(X)            roundf(X)
#define SqrtF32(X)             sqrtf(X)
#define Atan2F32(Y, X)         atan2f(Y, X)

//~ Vectors
union v2
{
   struct { f32 X, Y; };
   f32 Es[2];
};

union v2s
{
   struct { s32 X, Y; };
   s32 Es[2];
};

union color
{
   struct { u8 R, G, B, A; };
   u8 Es[4];
   u32 ColorU32;
};

internal v2 V2(f32 X, f32 Y);
inline internal v2  operator+ (v2 U, v2 V)   { return V2(U.X + V.X, U.Y + V.Y); }
inline internal v2  operator- (v2 U, v2 V)   { return V2(U.X - V.X, U.Y - V.Y); }
inline internal v2  operator- (v2 U)            { return V2(-U.X, -U.Y); }
inline internal v2  operator* (f32 Scale, v2 U) { return V2(Scale * U.X, Scale * U.Y); }
inline internal v2  operator* (v2 U, f32 Scale) { return V2(Scale * U.X, Scale * U.Y); }
inline internal v2  operator/ (v2 U, f32 Scale) { return (1.0f/Scale) * U; }
inline internal v2 &operator+=(v2 &U, v2 V)  { U.X += V.X; U.Y += V.Y; return U; }
inline internal v2 &operator-=(v2 &U, v2 V)  { U.X -= V.X; U.Y -= V.Y; return U; }
inline internal b32 operator==(v2 U, v2 V)   { return (U.X == V.X && U.Y == V.Y); }
inline internal b32 operator!=(v2 U, v2 V)   { return !(U == V); }

internal v2s V2S32(s32 X, s32 Y);
internal b32 operator==(v2s U, v2s V) { return (U.X == V.X && U.Y == V.Y); }
internal b32 operator!=(v2s U, v2s V) { return !(U == V); }

internal color MakeColor(u8 R, u8 G, u8 B, u8 A = 255);

read_only global color BlackColor       = MakeColor(0, 0, 0);
read_only global color WhiteColor       = MakeColor(255, 255, 255);
read_only global color GreenColor       = MakeColor(0, 255, 0);
read_only global color RedColor         = MakeColor(255, 0, 0);
read_only global color BlueColor        = MakeColor(0, 0, 255);
read_only global color YellowColor      = MakeColor(255, 255, 0);
read_only global color TransparentColor = MakeColor(0, 0, 0, 0);

#define V2FromVec(V) V2((f32)(V).x, (f32)(V).y)
#define V2S32FromVec(V) V2S32((s32)(V).x, (s32)(V).y)
#define ColorFromVec(V) MakeColor((u8)(V).r, (u8)(V).g, (u8)(V).b, (u8)(V).a)

typedef v2s screen_position;
typedef v2  camera_position;
typedef v2  world_position;
typedef v2  local_position;

//~ Calculations, algebra
inline internal f32 Cube(f32 X) { return X * X * X; }
inline internal f32 Square(f32 X) { return X * X; }

internal f32 Norm(v2 V);
internal f32 NormSquared(v2 V);
internal s32 NormSquared(v2s V);
internal void Normalize(v2 *V);
internal f32 Dot(v2 U, v2 V);
internal f32 Cross(v2 U, v2 V);

typedef u64 hull_point_count64;
internal hull_point_count64 CalcConvexHull(u64 PointCount, v2 *Points, v2 *OutPoints);

struct line_vertices
{
   u64 NumVertices;
   u64 CapVertices;
   sf::Vertex *Vertices;
   sf::PrimitiveType PrimitiveType;
};
enum line_vertices_allocation_type
{
   LineVerticesAllocation_None,
   LineVerticesAllocation_Arena,
};
struct line_vertices_allocation
{
   line_vertices_allocation_type Type;
   union {
      sf::Vertex *VerticesBuffer;
      line_vertices OldVertices;
      arena *Arena;
   };
};
internal line_vertices_allocation LineVerticesAllocationNone(sf::Vertex *VerticesBuffer);
internal line_vertices_allocation LineVerticesAllocationArena(arena *Arena);
internal line_vertices            CalculateLineVertices(u64 NumLinePoints, v2 *LinePoints,
                                                        f32 LineWidth, color LineColor, b32 Loop,
                                                        line_vertices_allocation Allocation);

internal color LerpColor(color From, color To, f32 T);

typedef v2 rotation_2d;
internal rotation_2d Rotation2D(f32 X, f32 Y);
internal rotation_2d Rotation2DZero(void);
internal rotation_2d Rotation2DFromVector(v2 Vector);
internal rotation_2d Rotation2DFromDegrees(f32 Degrees);
internal rotation_2d Rotation2DFromRadians(f32 Radians);
internal f32         Rotation2DToDegrees(rotation_2d Rotation);
internal f32         Rotation2DToRadians(rotation_2d Rotation);
internal rotation_2d Rotation2DInverse(rotation_2d Rotation);
internal rotation_2d Rotate90DegreesAntiClockwise(rotation_2d Rotation);
internal rotation_2d Rotate90DegreesClockwise(rotation_2d Rotation);
internal rotation_2d Rotation2DFromMovementAroundPoint(v2 From, v2 To, v2 Center);
internal v2          RotateAround(v2 Point, v2 Center, rotation_2d Rotation);
internal rotation_2d CombineRotations2D(rotation_2d RotationA, rotation_2d RotationB);

//~ Interpolation
internal void EquidistantPoints(f32 *Ti, u64 N);
internal void ChebychevPoints(f32 *Ti, u64 N);

internal void BarycentricOmega(f32 *Omega, f32 *Ti, u64 N);
internal void BarycentricOmegaWerner(f32 *Omega, f32 *Ti, u64 N);
internal void BarycentricOmegaEquidistant(f32 *Omega, f32 *Ti, u64 N);
internal void BarycentricOmegaChebychev(f32 *Omega, u64 N);
internal f32  BarycentricEvaluate(f32 T, f32 *Omega, f32 *Ti, f32 *Y, u64 N);

internal void NewtonBeta(f32 *Beta, f32 *Ti, f32 *Y, u64 N);
internal void NewtonBetaFast(f32 *Beta, f32 *Ti, f32 *Y, u64 N);
internal f32  NewtonEvaluate(f32 T, f32 *Beta, f32 *Ti, u64 N);

internal void CubicSplineNaturalM(f32 *M, f32 *Ti, f32 *Y, u64 N);
internal void CubicSplinePeriodicM(f32 *M, f32 *Ti, f32 *Y, u64 N);
internal f32  CubicSplineEvaluate(f32 T, f32 *M, f32 *Ti, f32 *Y, u64 N);

struct bezier_lower_degree
{
   b32 Failure;
   
   u64 MiddlePointIndex;
   
   v2 P_I;
   v2 P_II;
   
   f32 W_I;
   f32 W_II;
};

internal v2               BezierCurveEvaluate(f32 T, v2 *P, u64 N);
internal v2               BezierCurveEvaluateWeighted(f32 T, v2 *P, f32 *W, u64 N);
internal void                BezierCurveElevateDegree(v2 *P, u64 N);
internal void                BezierCurveElevateDegreeWeighted(v2 *P, f32 *W, u64 N);
internal bezier_lower_degree BezierCurveLowerDegree(v2 *P, f32 *W, u64 N);
internal void                BezierCubicCalculateAllControlPoints(u64 N, v2 *P, v2 *Output);
internal void                BezierCurveSplit(f32 T, u64 N, v2 *P, f32 *W, 
                                              v2 *LeftPoints, f32 *LeftWeights,
                                              v2 *RightPoints, f32 *RightWeights);

struct all_de_casteljau_intermediate_results
{
   u64 IterationCount;
   u64 TotalPointCount;
   // NOTE(hbr): Packed points: P1,P2,P3, Q1,Q2, R1
   v2 *P;
   f32 *W;
};
internal all_de_casteljau_intermediate_results DeCasteljauAlgorithm(arena *Arena, f32 T, v2 *P, f32 *W, u64 N);

internal void GaussianElimination(f32 *A, u64 N, f32 *Solution);

//~ Collisions, intersections, geometry
struct line_intersection
{
   b32 IsOneIntersection;
   v2 IntersectionPoint;
};

internal b32               PointCollision(v2 Position, v2 Point, f32 PointRadius);
internal b32               SegmentCollision(v2 Position, v2 LineA, v2 LineB, f32 LineWidth);
internal line_intersection LineIntersection(v2 A, v2 B, v2 C, v2 D);

#endif //EDITOR_MATH_H
