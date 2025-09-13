#ifndef EDITOR_MATH_H
#define EDITOR_MATH_H

#include <math.h>

#define PiF32    3.14159265358979323846f
#define EulerF32 2.71828182845904523536f
#define TauF32   6.28318530717958647692f

#define DegToRadF32 (PiF32 / 180.0f)
#define RadToDegF32 (180.0f / PiF32)

#define SinF32(X)              sinf(X)
#define CosF32(X)              cosf(X)
#define TanF32(X)              tanf(X)
#define AsinF32(X)             asinf(X)
#define AcosF32(X)             acosf(X)
#define AtanF32(X)             atanf(X)
#define Atan2F32(Y, X)         atan2f(Y, X)
#define FloorF32(X)            floorf(X)
#define CeilF32(X)             ceilf(X)
#define RoundF32(X)            roundf(X)
#define ModF32(X, M)           fmodf(X, M)
#define PowF32(Base, Exponent) powf(Base, Exponent)
#define SqrtF32(X)             sqrtf(X)
#define ExpF32(X)              expf(X)
#define LogF32(X)              logf(X)
#define LogBaseF32(Base, X)    (LogF32(X) / LogF32(Base))
#define Log10F32(X)            log10f(X)
#define TanhF32(X)             tanhf(X)
#define SinhF32(X)             sinhf(X)
#define CoshF32(X)             coshf(X)

//~ Basic
union rotation2d
{
 v2 V;
 struct { f32 X; f32 Y; };
};

union scale2d
{
 v2 V;
 struct { f32 X; f32 Y; };
};

struct xform2d
{
 v2 P;
 scale2d Scale;
 rotation2d Rotation;
};

union rgba
{
 v4 C;
 struct { f32 R; f32 G; f32 B; f32 A; };
};

union hsva
{
 v4 C;
 struct { f32 H; f32 S; f32 V; f32 A; };
};

internal rgba RGBA(f32 R, f32 G, f32 B, f32 A);
internal rgba RGBA_U8(u8 R, u8 G, u8 B, u8 A = 255);
internal rgba RGBA_V4(v4 C);
internal rgba RGB_Hex(u32 HexCode);
internal rgba RGBA_From_HSVA(hsva Color);
internal rgba RGBA_Gray(u8 Gray, u8 Alpha = 255);
internal rgba RGBA_Brighten(rgba Color, f32 BrightenByRatio);
internal rgba RGBA_Darken(rgba Color, f32 DarkenByRatio);
internal rgba RGBA_Fade(rgba Color, f32 FadeByRatio);
internal rgba RGBA_Opposite(rgba Color);

internal hsva HSVA(f32 H, f32 S, f32 V, f32 A);
internal hsva HSVA_Opposite(hsva Color);
internal hsva HSVA_From_RGBA(rgba Color);

read_only global rgba BlackColor       = RGBA(0.0f, 0.0f, 0.0f, 1.0f);
read_only global rgba WhiteColor       = RGBA(1.0f, 1.0f, 1.0f, 1.0f);
read_only global rgba GreenColor       = RGBA(0.0f, 1.0f, 0.0f, 1.0f);
read_only global rgba RedColor         = RGBA(1.0f, 0.0f, 0.0f, 1.0f);
read_only global rgba BlueColor        = RGBA(0.0f, 0.0f, 1.0f, 1.0f);
read_only global rgba YellowColor      = RGBA(1.0f, 1.0f, 0.0f, 1.0f);
read_only global rgba TransparentColor = RGBA(0.0f, 0.0f, 0.0f, 0.0f);

read_only global rgba NiceGreenColor = RGB_Hex(0x448724);
read_only global rgba NiceRedColor = RGB_Hex(0x902F23);

//- operations
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

inline internal b32 operator==(v2i U, v2i V) { return (U.X == V.X && U.Y == V.Y); }
inline internal b32 operator!=(v2i U, v2i V) { return !(U == V); }

inline internal v4 operator+(v4 U, v4 V) { return V4(U.X + V.X, U.Y + V.Y, U.Z + V.Z, U.W + V.W); }
inline internal v4 operator-(v4 U, v4 V) { return V4(U.X - V.X, U.Y - V.Y, U.Z - V.Z, U.W - V.W); }
inline internal v4 operator*(f32 Scale, v4 U) { return V4(Scale * U.X, Scale * U.Y, Scale * U.Z, Scale * U.W); }
inline internal v4 operator*(v4 U, f32 Scale) { return Scale * U; }
inline internal v4 &operator+=(v4 &U, v4 V) { U.X += V.X; U.Y += V.Y; U.Z += V.Z; U.W += V.W; return U; }
inline internal v4 &operator-=(v4 &U, v4 V) { U.X -= V.X; U.Y -= V.Y; U.Z -= V.Z; U.W -= V.W; return U; }
inline internal v4 &operator*=(v4 &U, f32 Scale) { U.X *= Scale; U.Y *= Scale; U.Z *= Scale; U.W *= Scale; return U; }

inline internal rgba operator+(rgba U, rgba V) { return RGBA_V4(U.C + V.C); }
inline internal rgba operator*(f32 Scale, rgba C) { return RGBA_V4(Scale * C.C); }
inline internal rgba operator*(rgba C, f32 Scale) { return RGBA_V4(C.C * Scale); }
inline internal rgba &operator*=(rgba &C, f32 Scale) { C.C *= Scale; return C; }

//~ Calculations, linear algebra
internal f32 Norm(v2 V);
internal f32 NormSquared(v2 V);
internal i32 NormSquared(v2i V);
internal void Normalize(v2 *V);
internal v2 Normalized(v2 V);
internal f32 Dot(v2 U, v2 V);
internal f32 Cross(v2 U, v2 V);
internal v2 Hadamard(v2 A, v2 B);
internal v2 ProjectOnto(v2 U, v2 Onto);

typedef u32 hull_point_count32;
internal hull_point_count32 CalcConvexHull(u32 PointCount, v2 *Points, v2 *OutPoints);

internal rotation2d Rotation2D(f32 X, f32 Y);
internal rotation2d Rotation2D(v2 V);
internal rotation2d Rotation2DZero(void);
internal rotation2d Rotation2DFromVector(v2 Vector);
internal rotation2d Rotation2DFromDegrees(f32 Degrees);
internal rotation2d Rotation2DFromRadians(f32 Radians);
internal f32 Rotation2DToDegrees(rotation2d Rotation);
internal f32 Rotation2DToRadians(rotation2d Rotation);
internal rotation2d Rotation2DInverse(rotation2d Rotation);
internal rotation2d Rotate90DegreesAntiClockwise(rotation2d Rotation);
internal rotation2d Rotate90DegreesClockwise(v2 Rotation);
internal rotation2d Rotation2DFromMovementAroundPoint(v2 From, v2 To, v2 Center);
internal v2 RotateAround(v2 Point, v2 Center, rotation2d Rotation);
internal rotation2d CombineRotations2D(rotation2d RotationA, rotation2d RotationB);

internal scale2d Scale2D(f32 X, f32 Y);
internal scale2d Scale2DOne(void);

internal xform2d XForm2D(v2 P, v2 Scale, v2 Rotation);
internal xform2d XForm2DZero(void);
internal xform2d XForm2DFromP(v2 P);

internal mat3 Identity3x3(void);
internal mat3 Transpose3x3(mat3 M);
internal mat3 Rows3x3(v2 X, v2 Y);
internal mat3 Cols3x3(v2 X, v2 Y);
internal mat3 Scale3x3(mat3 A, f32 Scale);
internal mat3 Scale3x3(mat3 A, v2 Scale);
internal mat3 Translate3x3(mat3 A, v2 P);
internal mat3 Diag3x3(f32 X, f32 Y);
internal mat3 Multiply3x3(mat3 A, mat3 B);
internal v3 Transform3x3(mat3 A, v3 P);

internal mat4 Identity4x4(void);
internal mat4 Scale4x4(mat4 A, f32 Scale);
internal mat4 Scale4x4(mat4 A, v3 Scale);
internal mat4 Translate4x4(mat4 A, v3 P);
internal mat4 Transpose4x4(mat4 A);
internal v4 Transform4x4(mat4 A, v4 P);

internal i32 GaussianElimination(f32 *A, f32 *B, u32 Rows, u32 Cols); // returns number of free variables (or -1 if no solution)

//~ Interpolation
internal void EquidistantPoints(f32 *Ti, u32 N, f32 A, f32 B);
internal void ChebychevPoints(f32 *Ti, u32 N);

// Barycentric form polynomial
internal void BarycentricOmega(f32 *Omega, f32 *Ti, u32 N);
internal void BarycentricOmegaWerner(f32 *Omega, f32 *Ti, u32 N);
internal void BarycentricOmegaEquidistant(f32 *Omega, f32 *Ti, u32 N);
internal void BarycentricOmegaChebychev(f32 *Omega, u32 N);
internal f32 BarycentricEvaluate(f32 T, f32 *Omega, f32 *Ti, f32 *Y, u32 N);

//- Newton form polynomial
internal void NewtonBeta(f32 *Beta, f32 *Ti, f32 *Y, u32 N);
internal void NewtonBetaFast(f32 *Beta, f32 *Ti, f32 *Y, u32 N);
internal f32  NewtonEvaluate(f32 T, f32 *Beta, f32 *Ti, u32 N);

//- Cubic Spline interpolation
internal void CubicSplineNaturalM(f32 *M, f32 *Ti, f32 *Y, u32 N);
internal void CubicSplinePeriodicM(f32 *M, f32 *Ti, f32 *Y, u32 N);
internal f32  CubicSplineEvaluate_Scalar(f32 T, f32 *M, f32 *Ti, f32 *Y, u32 N);

//- Bezier curve
struct bezier_lower_degree_inverse_degree_elevation
{
 b32 Failure;
 
 u32 MiddlePointIndex;
 
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

internal v2 BezierCurveEvaluate(f32 T, v2 *P, u32 N);
internal void BezierCurveRationalEvaluateScalar(f32 T[1], v2 *P, f32 *W, u32 N, v2 Out[1]);
internal void BezierCurveRationalEvaluateSSE(f32 T[4], v2 *P, f32 *W, u32 N, v2 Out[4]);
internal void BezierCurveRationalEvaluateAVX2(f32 T[8], v2 *P, f32 *W, u32 N, v2 Out[8]);
internal void BezierCurveRationalEvaluateAVX512(f32 T[16], v2 *P, f32 *W, u32 N, v2 Out[16]);

internal void                                         BezierCurveElevateDegree(v2 *P, u32 N);
internal void                                         BezierCurveRationalEvelateDegree(v2 *P, f32 *W, u32 N);
internal bezier_lower_degree_inverse_degree_elevation BezierCurveLowerDegreeUsingInverseDegreeElevation(v2 *P, f32 *W, u32 N);
internal void                                         BezierCurveLowerDegreeOptimalUniformNorm(v2 *P, f32 *W, u32 N);
internal void                                         BezierCubicCalculateAllControlPoints(u32 N, v2 *P, cubic_bezier_point *Out);
internal void                                         BezierCurveSplit(f32 T, u32 N, v2 *P, f32 *W, 
                                                                       v2 *LeftPoints, f32 *LeftWeights,
                                                                       v2 *RightPoints, f32 *RightWeights);

struct all_de_casteljau_intermediate_results
{
 u32 IterationCount;
 u32 TotalPointCount;
 // NOTE(hbr): Packed points: P1,P2,P3, Q1,Q2, R1
 v2 *P;
 f32 *W;
};
internal all_de_casteljau_intermediate_results DeCasteljauAlgorithm(arena *Arena, f32 T, v2 *P, f32 *W, u32 N);

//- B-Spline curve
struct b_spline_degree_bounds
{
 u32 MinDegree;
 u32 MaxDegree;
};

struct b_spline_knot_params
{
 u32 Degree; // this should be preserved in case of other numbers changed
 u32 PartitionSize;
 u32 KnotCount; // in general KnotCount = PartitionSize + 2*Degree
 f32 A;
 f32 B;
};

internal void                   BSplineBaseKnots(b_spline_knot_params KnotParams, f32 *Knots);
internal void                   BSplineKnotsNaturalExtension(b_spline_knot_params KnotParams, f32 *Knots);
internal void                   BSplineKnotsPeriodicExtension(b_spline_knot_params KnotParams, f32 *Knots);
internal v2                     BSplineEvaluate(f32 T, v2 *ControlPoints, b_spline_knot_params KnotParams, f32 *Knots);
internal b_spline_degree_bounds BSplineDegreeBounds(u32 ControlPointCount);
internal b_spline_knot_params   BSplineKnotParamsFromDegree(u32 Degree, u32 ControlPointCount);

// TODO(hbr): Inconsistent naming of variables but I don't have time now to change that
internal v2 NURBS_EvaluateScalar(f32 T, v2 *Controls, f32 *Weights, b_spline_knot_params KnotParams, f32 *Knots);
internal void NURBS_EvaluateSSE(v2 *P, f32 *W, u32 n, u32 m, f32 *Knots, f32 t4[4], v2 out[4]);
internal void NURBS_EvaluateAVX2(v2 *P, f32 *W, u32 n, u32 m, f32 *Knots, f32 t8[8], v2 out[8]);

//~ Collisions, intersections, geometry
struct line_intersection
{
 b32 IsOneIntersection;
 v2 IntersectionPoint;
};

internal b32               PointCollision(v2 Position, v2 Point, f32 PointRadius);
internal f32               PointDistanceSquaredSigned(v2 P, v2 Point, f32 Radius); // Dist^2 - R^2
internal b32               SegmentCollision(v2 Position, v2 LineA, v2 LineB, f32 LineWidth);
internal line_intersection LineIntersection(v2 A, v2 B, v2 C, v2 D);
internal f32               AABBSignedDistance(v2 P, v2 BoxP, v2 BoxSize);
internal f32               SegmentSignedDistance(v2 P, v2 SegmentBegin, v2 SegmentEnd, f32 SegmentWidth);
internal f32               TriangleArea(v2 P0, v2 P1, v2 P2);

//~ Misc
struct samples
{
 f32 SamplesMax;
 f32 SamplesMin;
 f32 SamplesSum;
 f32 SamplesSum2;
 f32 SamplesAvg;
 f32 SamplesStddev;
 u32 Count;
};

internal samples MakeSamples(void);
internal void    AddSample(samples *Set, f32 Sample);

#endif //EDITOR_MATH_H
