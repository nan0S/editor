#ifndef EDITOR_MATH_H
#define EDITOR_MATH_H

#include <math.h>

#define Pi32 3.14159265359f
#define DegToRad32 (Pi32 / 180.0f)
#define RadToDeg32 (180.0f / Pi32)

#define PowF32(Base, Exponent) powf(Base, Exponent)
#define SinF32(X)              sinf(X)
#define Log10F32(X)            log10f(X)
#define FloorF32(X)            floorf(X)
#define CeilF32(X)             ceilf(X)
#define RoundF32(X)            roundf(X)
#define SqrtF32(X)             sqrtf(X)
#define Atan2F32(Y, X)         atan2f(Y, X)

//~ Basic operations
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

inline internal b32 operator==(v2s U, v2s V) { return (U.X == V.X && U.Y == V.Y); }
inline internal b32 operator!=(v2s U, v2s V) { return !(U == V); }

inline internal v4 operator+(v4 U, v4 V) { return V4(U.X + V.X, U.Y + V.Y, U.Z + V.Z, U.W + V.W); }
inline internal v4 operator-(v4 U, v4 V) { return V4(U.X - V.X, U.Y - V.Y, U.Z - V.Z, U.W - V.W); }
inline internal v4 operator*(f32 Scale, v4 U) { return V4(Scale * U.X, Scale * U.Y, Scale * U.Z, Scale * U.W); }
inline internal v4 operator*(v4 U, f32 Scale) { return Scale * U; }
inline internal v4 &operator+=(v4 &U, v4 V) { U.X += V.X; U.Y += V.Y; U.Z += V.Z; U.W += V.W; return U; }
inline internal v4 &operator-=(v4 &U, v4 V) { U.X -= V.X; U.Y -= V.Y; U.Z -= V.Z; U.W -= V.W; return U; }
inline internal v4 &operator*=(v4 &U, f32 Scale) { U.X *= Scale; U.Y *= Scale; U.Z *= Scale; U.W *= Scale; return U; }

internal v4 RGBA_Color(u8 R, u8 G, u8 B, u8 A = 255);
internal v4 BrightenColor(v4 Color, f32 BrightenByRatio);
internal v4 DarkenColor(v4 Color, f32 DarkenByRatio);

read_only global v4 BlackColor       = V4(0.0f, 0.0f, 0.0f, 1.0f);
read_only global v4 WhiteColor       = V4(1.0f, 1.0f, 1.0f, 1.0f);
read_only global v4 GreenColor       = V4(0.0f, 1.0f, 0.0f, 1.0f);
read_only global v4 RedColor         = V4(1.0f, 0.0f, 0.0f, 1.0f);
read_only global v4 BlueColor        = V4(0.0f, 0.0f, 1.0f, 1.0f);
read_only global v4 YellowColor      = V4(1.0f, 1.0f, 0.0f, 1.0f);
read_only global v4 TransparentColor = V4(0.0f, 0.0f, 0.0f, 0.0f);

internal rectangle2 EmptyAABB(void);
internal void AddPointAABB(rectangle2 *AABB, v2 P);
internal b32 IsNonEmpty(rectangle2 *Rect);

//~ Calculations, linear algebra
inline internal f32 Cube(f32 X) { return X * X * X; }
inline internal f32 Square(f32 X) { return X * X; }

internal f32 Norm(v2 V);
internal f32 NormSquared(v2 V);
internal s32 NormSquared(v2s V);
internal void Normalize(v2 *V);
internal f32 Dot(v2 U, v2 V);
internal f32 Cross(v2 U, v2 V);
internal v2 Hadamard(v2 A, v2 B);

typedef u64 hull_point_count64;
internal hull_point_count64 CalcConvexHull(u64 PointCount, v2 *Points, v2 *OutPoints);

internal v2 Rotation2D(f32 X, f32 Y);
internal v2 Rotation2DZero(void);
internal v2 Rotation2DFromVector(v2 Vector);
internal v2 Rotation2DFromDegrees(f32 Degrees);
internal v2 Rotation2DFromRadians(f32 Radians);
internal f32 Rotation2DToDegrees(v2 Rotation);
internal f32 Rotation2DToRadians(v2 Rotation);
internal v2 Rotation2DInverse(v2 Rotation);
internal v2 Rotate90DegreesAntiClockwise(v2 Rotation);
internal v2 Rotate90DegreesClockwise(v2 Rotation);
internal v2 Rotation2DFromMovementAroundPoint(v2 From, v2 To, v2 Center);
internal v2 RotateAround(v2 Point, v2 Center, v2 Rotation);
internal v2 CombineRotations2D(v2 RotationA, v2 RotationB);

inline internal transform operator*(transform T2, transform T1);
internal transform Identity(void);
internal transform_inv MakeFullTransform(v2 P, v2 Rotation, v2 Scale);
internal v2 Transform(transform A, v2 P);
internal v2 Project(transform_inv *XForm, v2 P);
internal v2 Unproject(transform_inv *XForm, v2 P);
internal v2 TransformLength(transform A, v2 Length);
internal v2 ProjectLength(transform_inv *XForm, v2 Length);
internal v2 UnprojectLength(transform_inv *XForm, v2 Length);

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

union cubic_bezier_point
{
 struct {
  v2 P0;
  v2 P1;
  v2 P2;
 };
 v2 Ps[3];
};

internal v2                  BezierCurveEvaluate(f32 T, v2 *P, u64 N);
internal v2                  BezierCurveEvaluateWeighted(f32 T, v2 *P, f32 *W, u64 N);
internal void                BezierCurveElevateDegree(v2 *P, u64 N);
internal void                BezierCurveElevateDegreeWeighted(v2 *P, f32 *W, u64 N);
internal bezier_lower_degree BezierCurveLowerDegree(v2 *P, f32 *W, u64 N);
internal void                BezierCubicCalculateAllControlPoints(u64 N, v2 *P, cubic_bezier_point *Out);
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
