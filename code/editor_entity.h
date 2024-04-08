#ifndef EDITOR_ENTITY_H
#define EDITOR_ENTITY_H

//- Name String
struct name_string
{
   char Str[100];
   u64 Size;
};

function name_string NameStringMake(char *Str, u64 Size);
function name_string NameStringFromString(string String);
#define NameStringFromLiteral(Literal) NameStringMake(cast(char *)Literal, ArrayCount(Literal)-1)
function name_string NameStringFormat(char const *Format, ...);

//- Curve
enum interpolation_type
{
   Interpolation_Polynomial,
   Interpolation_CubicSpline,
   Interpolation_Bezier,
};

enum polynomial_interpolation_type
{
   PolynomialInterpolation_Barycentric,
   PolynomialInterpolation_Newton,
};

enum points_arrangement
{
   PointsArrangement_Equidistant,
   PointsArrangement_Chebychev,
};

struct polynomial_interpolation_params
{
   polynomial_interpolation_type Type;
   points_arrangement PointsArrangement;
};

enum cubic_spline_type
{
   CubicSpline_Natural,
   CubicSpline_Periodic,
};

enum bezier_type
{
   Bezier_Normal,
   Bezier_Weighted,
   Bezier_Cubic,
};

struct curve_shape
{
   interpolation_type InterpolationType;
   polynomial_interpolation_params PolynomialInterpolationParams;
   cubic_spline_type CubicSplineType;
   bezier_type BezierType;
};

struct curve_params
{
   curve_shape CurveShape;
   color CurveColor;
   f32 CurveWidth;
   
   b32 PointsDisabled;
   color PointColor;
   f32 PointSize;
   
   b32 PolylineEnabled;
   color PolylineColor;
   f32 PolylineWidth;
   
   b32 ConvexHullEnabled;
   color ConvexHullColor;
   f32 ConvexHullWidth;
   
   u64 NumCurvePointsPerSegment;
   
   struct {
      color GradientA;
      color GradientB;
   } DeCasteljau;
   
   b32 Hidden;
   
   b32 CubicBezierHelpersDisabled;
};

struct curve
{
   name_string Name;
   
   curve_params CurveParams;
   
   world_position Position;
   rotation_2d Rotation;
   
   u64 NumControlPoints;
   u64 CapControlPoints;
   local_position *ControlPoints;
   
   // NOTE(hbr): Weighted Bezier case
   // TODO(hbr): Remove capacity
   u64 CapControlPointWeights;
   f32 *ControlPointWeights;
   
   u64 CapCubicBezierPoints;
   local_position *CubicBezierPoints;
   
   u64 NumCurvePoints;
   local_position *CurvePoints;
   
   line_vertices CurveVertices;
   line_vertices PolylineVertices;
   line_vertices ConvexHullVertices;
   
   b32 IsSelected;
   u64 SelectedControlPointIndex;
   
   u64 IsRenamingFrame;
   
   u64 CurveVersion; // NOTE(hbr): Changes every recomputation
};

function curve_shape CurveShapeMake(interpolation_type InterpolationType,
                                    polynomial_interpolation_type PolynomialInterpolationType,
                                    points_arrangement PointsArrangement,
                                    cubic_spline_type CubicSplineType,
                                    bezier_type BezierType);

function curve_params CurveParamsMake(curve_shape CurveShape,
                                      f32 CurveWidth, color CurveColor,
                                      b32 PointsDisabled, f32 PointSize, color PointColor,
                                      b32 PolylineEnabled, f32 PolylineWidth, color PolylineColor,
                                      b32 ConvexHullEnabled, f32 ConvexHullWidth, color ConvexHullColor,
                                      u64 NumCurvePointsPerSegment, color DeCasteljauVisualizationGradientA,
                                      color DeCasteljauVisualizationGradientB);

function curve CurveMake(name_string CurveName,
                         curve_params CurveParams,
                         u64 SelectedControlPointIndex,
                         world_position Position,
                         rotation_2d Rotation);
function void CurveDestroy(curve *Curve);
function curve CurveCopy(curve Curve);

typedef u64 added_point_index;
function added_point_index CurveAppendControlPoint(curve *Curve, world_position ControlPoint,
                                                   f32 ControlPointWeight);

function void CurveInsertControlPoint(curve *Curve, world_position ControlPoint, u64 InsertAfterIndex,
                                      f32 ControlPointWeight);
function void CurveRemoveControlPoint(curve *Curve, u64 ControlPointIndex);
function void CurveSetControlPoints(curve *Curve, u64 NewNumControlPoints, local_position *NewControlPoints,
                                    f32 *NewControlPointWeights, local_position *NewCubicBezierPoints);
function void CurveTranslatePoint(curve *Curve, u64 PointIndex, b32 IsCubicBezierPoint,
                                  b32 MatchCubicBezierTwinDirection, b32 MatchCubicBezierTwinLength,
                                  v2f32 TranslationWorld);
function void CurveSetControlPoint(curve *Curve, u64 ControlPointIndex, local_position ControlPoint,
                                   f32 ControlPointWeight);
function void CurveRotateAround(curve *Curve, world_position Center, rotation_2d Rotation);
function void CurveRecompute(curve *Curve);
function void CurveEvaluate(curve *Curve, u64 NumOutputCurvePoints, v2f32 *OutputCurvePoints);

function sf::Transform CurveGetAnimate(curve *Curve);
function b32 CurveIsLooped(curve_shape CurveShape);

function local_position WorldToLocalCurvePosition(curve *Curve, world_position Position);
function world_position LocalCurvePositionToWorld(curve *Curve, local_position Position);

//- Image
struct image
{
   name_string Name;
   
   world_position Position;
   v2f32 Scale;
   rotation_2d Rotation;
   
   s64 SortingLayer;
   
   b32 Hidden;
   u64 IsRenamingFrame;
   b32 IsSelected;
   
   string FilePath;
   sf::Texture Texture;
};

function image ImageMake(name_string Name, world_position Position,
                         v2f32 Scale, rotation_2d Rotation,
                         s64 SoritingLayer, b32 Hidden,
                         string FilePath, sf::Texture Texture);
function image ImageCopy(image Image);
function void ImageDestroy(image *Image);

function sf::Texture LoadTextureFromFile(arena *Arena, string FilePath,
                                         error_string *OutError);

struct image_sort_entry
{
   image *Image;
   u64 OriginalOrder;
};
struct sorting_layer_sorted_images
{
   u64 NumImages;
   image_sort_entry *SortedImages;
};
function sorting_layer_sorted_images SortingLayerSortedImages(arena *Arena,
                                                              u64 NumEntities,
                                                              image *EntityList);

//- Entity
enum entity_type
{
   Entity_Curve,
   Entity_Image,
};

struct entity
{
   entity *Prev;
   entity *Next;
   
   entity_type Type;
   
   // TODO(hbr): merge things being there and there
   curve Curve;
   image Image;
};

// TODO(hbr): This should be temporary
#define EntityFromCurve(CurvePtr) EnclosingTypeAddr(entity, Curve, CurvePtr)
#define EntityFromImage(ImagePtr) EnclosingTypeAddr(entity, Image, ImagePtr)

function entity CurveEntity(curve Curve);
function entity ImageEntity(image Image);
function void EntityDestroy(entity *Entity);

#endif //EDITOR_ENTITY_H