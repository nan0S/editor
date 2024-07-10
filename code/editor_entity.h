#ifndef EDITOR_ENTITY_H
#define EDITOR_ENTITY_H

struct name_string
{
   char Data[64];
   u64 Count;
};

internal name_string NameStr(char *Data, u64 Count);
internal name_string StrToNameStr(string String);
#define NameStrLit(Literal) NameStr(Cast(char *)Literal, ArrayCount(Literal) - 1)
internal name_string NameStrF(char const *Fmt, ...);

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
   
   u64 CurvePointCountPerSegment;
   
   struct {
      color GradientA;
      color GradientB;
   } DeCasteljau;
   
   b32 CubicBezierHelpersDisabled;
};

#define MAX_CONTROL_POINT_COUNT 1024
struct curve
{
   curve_params CurveParams;
   u64 SelectedControlPointIndex;
   
   u64 ControlPointCount;
   local_position ControlPoints[MAX_CONTROL_POINT_COUNT];
   f32 ControlPointWeights[MAX_CONTROL_POINT_COUNT];
   local_position CubicBezierPoints[3 * MAX_CONTROL_POINT_COUNT];
   
   arena *ComputationArena;
   u64 CurvePointCount;
   local_position *CurvePoints;
   line_vertices CurveVertices;
   line_vertices PolylineVertices;
   line_vertices ConvexHullVertices;
   u64 CurveVersion; // NOTE(hbr): Changes every recomputation
};

struct image
{
   string FilePath;
   sf::Texture Texture;
};

enum entity_type
{
   Entity_Curve,
   Entity_Image,
};

typedef u64 entity_flags;
enum
{
   EntityFlag_Hidden = (1<<0),
   EntityFlag_Selected = (1<<1),
};

struct entity
{
   entity *Prev;
   entity *Next;
   
   world_position Position;
   v2f32 Scale;
   rotation_2d Rotation;
   name_string Name;
   s64 SortingLayer;
   u64 RenamingFrame;
   entity_flags Flags;
   
   entity_type Type;
   curve Curve;
   image Image;
};

enum
{
   TranslateControlPoint_BezierPoint               = (1<<0),
   TranslateControlPoint_MatchBezierTwinDirection  = (1<<1),
   TranslateControlPoint_MatchBezierTwinLength     = (1<<2),
};
typedef u64 translate_control_point_flags;

internal curve *
GetCurve(entity *Entity)
{
   curve *Result = 0;
   if (Entity)
   {
      Assert(Entity->Type == Entity_Curve);
      Result = &Entity->Curve;
   }
   
   return Result;
}

internal image *
GetImage(entity *Entity)
{
   Assert(Entity->Type == Entity_Image);
   return &Entity->Image;
}

// TODO(hbr): I'm not sure if this internal belongs here, in this file
internal sf::Texture
LoadTextureFromFile(arena *Arena, string FilePath, error_string *OutError)
{
   sf::Texture Texture;
   
   temp_arena Temp = TempArena(Arena);
   string TextureData = OS_ReadEntireFile(Temp.Arena, FilePath);
   if (TextureData.Data)
   {
      if (!Texture.loadFromMemory(TextureData.Data, TextureData.Count))
      {
         *OutError = StrF(Arena,
                          "failed to load texture from file %s",
                          FilePath);
      }
   }
   else
   {
      *OutError = StrF(Arena, "failed to load texture");
   }
   
   EndTemp(Temp);
   
   return Texture;
}

// TODO(hbr): Remove this, why the heck this is necessary?!!!
internal void ChebyshevPoints(f32 *Ti, u64 N);

// TODO(hbr): Rename this internal and also maybe others to [EntityRotateAround] because it's not [curve] specific.
internal void
CurveRotateAround(entity *CurveEntity, world_position Center, rotation_2d Rotation)
{
   CurveEntity->Position = RotateAround(CurveEntity->Position, Center, Rotation);
   CurveEntity->Rotation = CombineRotations2D(CurveEntity->Rotation, Rotation);
}

internal curve_params
CurveParamsMake(curve_shape CurveShape,
                f32 CurveWidth, color CurveColor,
                b32 PointsDisabled, f32 PointSize,
                color PointColor, b32 PolylineEnabled,
                f32 PolylineWidth, color PolylineColor,
                b32 ConvexHullEnabled, f32 ConvexHullWidth,
                color ConvexHullColor, u64 CurvePointCountPerSegment,
                color DeCasteljauVisualizationGradientA,
                color DeCasteljauVisualizationGradientB)
{
   curve_params Result = {};
   Result.CurveShape = CurveShape;
   Result.CurveWidth = CurveWidth;
   Result.CurveColor = CurveColor;
   Result.PointsDisabled = PointsDisabled;
   Result.PointSize = PointSize;
   Result.PointColor = PointColor;
   Result.PolylineEnabled = PolylineEnabled;
   Result.PolylineWidth = PolylineWidth;
   Result.PolylineColor = PolylineColor;
   Result.ConvexHullEnabled = ConvexHullEnabled;
   Result.ConvexHullWidth = ConvexHullWidth;
   Result.ConvexHullColor = ConvexHullColor;
   Result.CurvePointCountPerSegment = CurvePointCountPerSegment;
   Result.DeCasteljau.GradientA = DeCasteljauVisualizationGradientA;
   Result.DeCasteljau.GradientB = DeCasteljauVisualizationGradientB;
   
   return Result;
}

internal curve_shape
CurveShapeMake(interpolation_type InterpolationType,
               polynomial_interpolation_type PolynomialInterpolationType,
               points_arrangement PointsArrangement,
               cubic_spline_type CubicSplineType,
               bezier_type BezierType)
{
   curve_shape Result = {};
   Result.InterpolationType = InterpolationType;
   Result.PolynomialInterpolationParams.Type = PolynomialInterpolationType;
   Result.PolynomialInterpolationParams.PointsArrangement = PointsArrangement;
   Result.CubicSplineType = CubicSplineType;
   Result.BezierType = BezierType;
   
   return Result;
}

struct entity_sort_entry
{
   entity *Entity;
   u64 OriginalOrder;
};
struct sorted_entity_array
{
   u64 EntityCount;
   entity_sort_entry *Entries;
};
internal sorted_entity_array SortEntitiesBySortingLayer(arena *Arena, entity *Entities);

internal int
EntitySortEntryCmp(entity_sort_entry *A, entity_sort_entry *B)
{
   int Result = 0;
   int Cmp1 = IntCmp(A->Entity->SortingLayer, B->Entity->SortingLayer);
   if (Cmp1 == 0)
   {
      int Cmp2 = IntCmp(A->OriginalOrder, B->OriginalOrder);
      Result = Cmp2;
   }
   else
   {
      Result = Cmp1;
   }
   
   return Result;
}

internal sorted_entity_array
SortEntitiesBySortingLayer(arena *Arena, entity *Entities)
{
   sorted_entity_array Result = {};
   
   ListIter(Entity, Entities, entity) { ++Result.EntityCount; }
   Result.Entries = PushArrayNonZero(Arena, Result.EntityCount, entity_sort_entry);
   
   {
      entity_sort_entry *Entry = Result.Entries;
      u64 Index = 0;
      ListIter(Entity, Entities, entity)
      {
         Entry->Entity = Entity;
         Entry->OriginalOrder = Index;
         ++Entry;
         ++Index;
      }
   }
   
   // NOTE(hbr): Compare internal makes this sort stable, and we need stable property
   QuickSort(Result.Entries, Result.EntityCount, entity_sort_entry, EntitySortEntryCmp);
   
   return Result;
}

// TODO(hbr): Why the fuck this is the name of this internal
// TODO(hbr): Rewrite this internal
internal sf::Transform
CurveGetAnimate(entity *Curve)
{
   sf::Transform Result =
      sf::Transform()
      .translate(V2F32ToVector2f(Curve->Position))
      .rotate(Rotation2DToDegrees(Curve->Rotation));
   
   return Result;
}

internal name_string
NameStr(char *Data, u64 Count)
{
   name_string Result = {};
   
   u64 FinalCount = Min(Count, ArrayCount(Result.Data) - 1);
   MemoryCopy(Result.Data, Data, FinalCount);
   Result.Data[FinalCount] = 0;
   Result.Count = FinalCount;
   
   return Result;
}

internal name_string
StrToNameStr(string String)
{
   name_string Result = NameStr(String.Data, String.Count);
   return Result;
}

internal name_string
NameStrF(char const *Fmt, ...)
{
   name_string Result = {};
   
   va_list Args;
   DeferBlock(va_start(Args, Fmt), va_end(Args))
   {
      int Return = vsnprintf(Result.Data,
                             ArrayCount(Result.Data) - 1,
                             Fmt, Args);
      if (Return >= 0)
      {
         // TODO(hbr): What the fuck is wrong with this internal
         Result.Count = CStrLen(Result.Data);
      }
   }
   
   return Result;
}

internal void
EntityDestroy(entity *Entity)
{
   switch (Entity->Type)
   {
      case Entity_Curve: {
         curve *Curve = &Entity->Curve;
         ArrayFree(Curve->ControlPoints);
         ArrayFree(Curve->ControlPointWeights);
         ArrayFree(Curve->CubicBezierPoints);
         ArrayFree(Curve->CurvePoints);
         ArrayFree(Curve->CurveVertices.Vertices);
         ArrayFree(Curve->PolylineVertices.Vertices);
         ArrayFree(Curve->ConvexHullVertices.Vertices);
      } break;
      
      case Entity_Image: {
         image *Image = &Entity->Image;
         Image->Texture.~Texture();
         FreeStr(&Image->FilePath);
      } break;
   }
   
   ZeroStruct(Entity);
}

#endif //EDITOR_ENTITY_H