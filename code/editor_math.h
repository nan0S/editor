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

//- Vectors
union v2f32
{
   struct { f32 X, Y; };
   f32 Es[2];
};

union v2s32
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

function v2f32 V2F32(f32 X, f32 Y);
inline function v2f32 operator+(v2f32 U, v2f32 V) { return V2F32(U.X + V.X, U.Y + V.Y); }
inline function v2f32 operator-(v2f32 U, v2f32 V) { return V2F32(U.X - V.X, U.Y - V.Y); }
inline function v2f32 operator-(v2f32 U) { return V2F32(-U.X, -U.Y); }
inline function v2f32 operator*(f32 Scale, v2f32 U) { return V2F32(Scale * U.X, Scale * U.Y); }
inline function v2f32 operator/(v2f32 U, f32 Scale) { return (1.0f/Scale) * U; }
inline function v2f32 &operator+=(v2f32 &U, v2f32 V) { U.X += V.X; U.Y += V.Y; return U; }
inline function v2f32 &operator-=(v2f32 &U, v2f32 V) { U.X -= V.X; U.Y -= V.Y; return U; }
inline function b32 operator==(v2f32 U, v2f32 V) { return (U.X == V.X && U.Y == V.Y); }
inline function b32 operator!=(v2f32 U, v2f32 V) { return !(U == V); }

function v2s32 V2S32(s32 X, s32 Y);
function b32 operator==(v2s32 U, v2s32 V) { return (U.X == V.X && U.Y == V.Y); }
function b32 operator!=(v2s32 U, v2s32 V) { return !(U == V); }

function color ColorMake(u8 R, u8 G, u8 B, u8 A = 255);

read_only global color BlackColor       = ColorMake(0, 0, 0);
read_only global color WhiteColor       = ColorMake(255, 255, 255);
read_only global color GreenColor       = ColorMake(0, 255, 0);
read_only global color RedColor         = ColorMake(255, 0, 0);
read_only global color BlueColor        = ColorMake(0, 0, 255);
read_only global color YellowColor      = ColorMake(255, 255, 0);
read_only global color TransparentColor = ColorMake(0, 0, 0, 0);

#define V2F32FromVec(V) V2F32((f32)(V).x, (f32)(V).y)
#define V2S32FromVec(V) V2S32((s32)(V).x, (s32)(V).y)
#define ColorFromVec(V) ColorMake((u8)(V).r, (u8)(V).g, (u8)(V).b, (u8)(V).a)

typedef v2s32 screen_position;
typedef v2f32 camera_position;
typedef v2f32 world_position;
typedef v2f32 local_position;

//- Calculations, algebra
inline function f32 Cube(f32 X) { return X * X * X; }
inline function f32 Square(f32 X) { return X * X; }

function f32 Norm(v2f32 V);
function f32 NormSquared(v2f32 V);
function s32 NormSquared(v2s32 V);
function void Normalize(v2f32 *V);
function f32 Dot(v2f32 U, v2f32 V);
function f32 Cross(v2f32 U, v2f32 V);

typedef u64 num_convex_hull_points;
function num_convex_hull_points CalculateConvexHull(u64 NumPoints, v2f32 *Points,
                                                    v2f32 *OutputConvexHullPoints);

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
   LineVerticesAllocation_Heap,
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
function line_vertices_allocation LineVerticesAllocationNone(sf::Vertex *VerticesBuffer);
function line_vertices_allocation LineVerticesAllocationHeap(line_vertices OldVertices);
function line_vertices_allocation LineVerticesAllocationArena(arena *Arena);
function line_vertices CalculateLineVertices(u64 NumLinePoints, v2f32 *LinePoints,
                                             f32 LineWidth, color LineColor, b32 Loop,
                                             line_vertices_allocation Allocation);

function f32 LerpF32(f32 From, f32 To, f32 T);
function v2f32 LerpV2F32(v2f32 From, v2f32 To, f32 T);
function color LerpColor(color From, color To, f32 T);

typedef v2f32 rotation_2d;
function rotation_2d Rotation2D(f32 X, f32 Y);
function rotation_2d Rotation2DZero(void);
function rotation_2d Rotation2DFromVector(v2f32 Vector);
function rotation_2d Rotation2DFromDegrees(f32 Degrees);
function rotation_2d Rotation2DFromRadians(f32 Radians);
function f32 Rotation2DToDegrees(rotation_2d Rotation);
function f32 Rotation2DToRadians(rotation_2d Rotation);
function rotation_2d Rotation2DInverse(rotation_2d Rotation);
function rotation_2d Rotate90DegreesAntiClockwise(rotation_2d Rotation);
function rotation_2d Rotate90DegreesClockwise(rotation_2d Rotation);
function rotation_2d Rotation2DFromMovementAroundPoint(v2f32 From, v2f32 To, v2f32 Center);
function v2f32 RotateAround(v2f32 Point, v2f32 Center, rotation_2d Rotation);
function rotation_2d CombineRotations2D(rotation_2d RotationA, rotation_2d RotationB);

//- Interpolation
function void EquidistantPoints(f32 *Ti, u64 N);
function void ChebychevPoints(f32 *Ti, u64 N);

function void BarycentricOmega(f32 *Omega, f32 *Ti, u64 N);
function void BarycentricOmegaWerner(f32 *Omega, f32 *Ti, u64 N);
function void BarycentricOmegaEquidistant(f32 *Omega, f32 *Ti, u64 N);
function void BarycentricOmegaChebychev(f32 *Omega, u64 N);
function f32 BarycentricEvaluate(f32 T, f32 *Omega, f32 *Ti, f32 *Y, u64 N);

function void NewtonBeta(f32 *Beta, f32 *Ti, f32 *Y, u64 N);
function void NewtonBetaFast(f32 *Beta, f32 *Ti, f32 *Y, u64 N);
function f32 NewtonEvaluate(f32 T, f32 *Beta, f32 *Ti, u64 N);

function void CubicSplineNaturalM(f32 *M, f32 *Ti, f32 *Y, u64 N);
function void CubicSplinePeriodicM(f32 *M, f32 *Ti, f32 *Y, u64 N);
function f32 CubicSplineEvaluate(f32 T, f32 *M, f32 *Ti, f32 *Y, u64 N);

struct bezier_lower_degree
{
   b32 Failure;
   
   u64 MiddlePointIndex;
   
   v2f32 P_I;
   v2f32 P_II;
   
   f32 W_I;
   f32 W_II;
};

function v2f32 BezierCurveEvaluate(f32 T, v2f32 *P, u64 N);
function v2f32 BezierWeightedCurveEvaluate(f32 T, v2f32 *P, f32 *W, u64 N);
function v2f32 BezierCurveEvaluateFast(f32 T, v2f32 *P, u64 N);
function v2f32 BezierWeightedCurveEvaluateFast(f32 T, v2f32 *P, f32 *W, u64 N);
function void BezierCurveElevateDegree(v2f32 *P, u64 N);
function void BezierWeightedCurveElevateDegree(v2f32 *P, f32 *W, u64 N);
function bezier_lower_degree BezierCurveLowerDegree(v2f32 *P, u64 N);
function bezier_lower_degree BezierWeightedCurveLowerDegree(v2f32 *P, f32 *W, u64 N);
function void BezierCubicCalculateAllControlPoints(v2f32 *P, u64 N, v2f32 *Output);
function void BezierCurveSplit(f32 T, v2f32 *P, f32 *W, u64 N,
                               v2f32 *LeftControlPoints, f32 *LeftControlPointWeights,
                               v2f32 *RightControlPoints, f32 *RightControlPointWeights);

function void DeCasteljauAlgorithm(f32 T, v2f32 *P, f32 *W, u64 N,
                                   v2f32 *OutputP, f32 *OutputW);

function void GaussianElimination(f32 *A, u64 N, f32 *Solution);

//- Collisions, intersections, geometry
struct line_intersection
{
   b32 IsOneIntersection;
   v2f32 IntersectionPoint;
};

function b32 PointCollision(v2f32 Position, v2f32 Point, f32 PointRadius);
function b32 SegmentCollision(v2f32 Position, v2f32 LineA, v2f32 LineB, f32 LineWidth);
function line_intersection LineIntersection(v2f32 A, v2f32 B, v2f32 C, v2f32 D);

#endif //EDITOR_MATH_H
