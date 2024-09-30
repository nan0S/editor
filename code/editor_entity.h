#ifndef EDITOR_ENTITY_H
#define EDITOR_ENTITY_H

enum interpolation_type : u8
{
   Interpolation_CubicSpline,
   Interpolation_Bezier,
   Interpolation_Polynomial,
   Interpolation_Count
};
// TODO(hbr): Try to add designated array intializers
read_only global char const *InterpolationNames[] = { "Cubic Spline", "Bezier", "Polynomial" };
StaticAssert(ArrayCount(InterpolationNames) == Interpolation_Count, InterpolationNamesDefined);

enum polynomial_interpolation_type : u8
{
   PolynomialInterpolation_Barycentric,
   PolynomialInterpolation_Newton,
   PolynomialInterpolation_Count,
};
read_only global char const *PolynomialInterpolationNames[] = { "Barycentric", "Newton" };
StaticAssert(ArrayCount(PolynomialInterpolationNames) == PolynomialInterpolation_Count, PolynomialInterpolationNamesDefined);

enum points_arrangement : u8
{
   PointsArrangement_Chebychev,
   PointsArrangement_Equidistant,
   PointsArrangement_Count,
};
read_only global char const *PointsArrangementNames[] = { "Chebychev", "Equidistant" };
StaticAssert(ArrayCount(PointsArrangementNames) == PointsArrangement_Count, PointsArrangementNamesDefined);

struct polynomial_interpolation_params
{
   polynomial_interpolation_type Type;
   points_arrangement PointsArrangement;
};

enum cubic_spline_type : u8
{
   CubicSpline_Natural,
   CubicSpline_Periodic,
   CubicSpline_Count,
};
read_only global char const *CubicSplineNames[] = { "Natural", "Periodic" };
StaticAssert(ArrayCount(CubicSplineNames) == CubicSpline_Count, CubicSplineNamesDefined);

enum bezier_type : u8
{
   Bezier_Regular,
   Bezier_Cubic,
   Bezier_Count,
};
read_only global char const *BezierNames[] = { "Regular", "Cubic" };
StaticAssert(ArrayCount(BezierNames) == Bezier_Count, BezierNamesDefined);

struct curve_params
{
   interpolation_type InterpolationType;
   polynomial_interpolation_params PolynomialInterpolationParams;
   cubic_spline_type CubicSplineType;
   bezier_type BezierType;
   
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
};

struct curve_point_tracking_state
{
   b32 Active;
   b32 NeedsRecomputationThisFrame;
   f32 T;
   local_position TrackedPoint;
   
   b32 IsSplitting;
   
   // NOTE(hbr): De Casteljau visualization
   arena *Arena;
   all_de_casteljau_intermediate_results Intermediate;
   color *IterationColors;
   line_vertices *LineVerticesPerIteration;
};

struct curve_degree_lowering_state
{
   b32 Active;
   
   arena *Arena;
   
   local_position *SavedControlPoints;
   f32 *SavedControlPointWeights;
   local_position *SavedCubicBezierPoints;
   
   // TODO(hbr): Use line vertices here instead?
   u64 NumSavedCurveVertices;
   sf::Vertex *SavedCurveVertices;
   sf::PrimitiveType SavedPrimitiveType;
   
   bezier_lower_degree LowerDegree;
   f32 MixParameter;
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
   
   u64 CurvePointCount;
   local_position *CurvePoints;
   line_vertices CurveVertices;
   line_vertices PolylineVertices;
   line_vertices ConvexHullVertices;
   
   b32 RecomputeRequested;
   
   curve_point_tracking_state PointTracking;
   curve_degree_lowering_state DegreeLowering;
};

struct image
{
   string ImagePath;
   sf::Texture Texture;
};

enum entity_type
{
   Entity_Curve,
   Entity_Image,
};

enum
{
   EntityFlag_Active   = (1<<0),
   EntityFlag_Hidden   = (1<<1),
   EntityFlag_Selected = (1<<2),
};
typedef u64 entity_flags;

struct entity
{
   arena *Arena;
   
   world_position Position;
   v2f32 Scale;
   rotation_2d Rotation;
   
   char NameBuffer[64];
   string Name;
   
   s64 SortingLayer;
   u64 RenamingFrame;
   entity_flags Flags;
   
   entity_type Type;
   curve Curve;
   image Image;
};

// TODO(hbr): Move this struct into [editor], it's only here because of compilation issues. So something about this type of shit in general
struct entities
{
   entity Entities[1024];
   u64 EntityCount;
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

// TODO(hbr): Rename this internal and also maybe others to [EntityRotateAround] because it's not [curve] specific.
internal void
CurveRotateAround(entity *CurveEntity, world_position Center, rotation_2d Rotation)
{
   CurveEntity->Position = RotateAround(CurveEntity->Position, Center, Rotation);
   CurveEntity->Rotation = CombineRotations2D(CurveEntity->Rotation, Rotation);
}

internal curve_params
DefaultCurveParams(void)
{
   curve_params Result = {};
   f32 CurveWidth = 0.009f;
   Result.CurveColor = MakeColor(21, 69, 98);
   Result.CurveWidth = CurveWidth;
   Result.PointColor = MakeColor(0, 138, 138, 148);
   Result.PointSize = 0.014f;
   color PolylineColor = MakeColor(16, 31, 31, 200);
   Result.PolylineColor = PolylineColor;
   Result.PolylineWidth = CurveWidth;
   Result.ConvexHullColor = PolylineColor;
   Result.ConvexHullWidth = CurveWidth;
   Result.CurvePointCountPerSegment = 50;
   
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
SortEntitiesByLayer(arena *Arena, entities *Entities)
{
   sorted_entity_array Result = {};
   Result.Entries = PushArrayNonZero(Arena, Entities->EntityCount, entity_sort_entry);
   u64 Index = 0;
   for (u64 EntityIndex = 0;
        EntityIndex < ArrayCount(Entities->Entities);
        ++EntityIndex)
   {
      entity *Entity = Entities->Entities + EntityIndex;
      if (Entity->Flags & EntityFlag_Active)
      {
         entity_sort_entry *Entry = Result.Entries + Index++;
         Entry->Entity = Entity;
         Entry->OriginalOrder = EntityIndex;
      }
   }
   Result.EntityCount = Index;
   Assert(Index <= Entities->EntityCount);
   
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

internal void
SetEntityName(entity *Entity, string Name)
{
   u64 ToCopy = Min(Name.Count, ArrayCount(Entity->NameBuffer) - 1);
   MemoryCopy(Entity->NameBuffer, Name.Data, ToCopy);
   Entity->NameBuffer[ToCopy] = 0;
   Entity->Name = MakeStr(Entity->NameBuffer, ToCopy);
}

internal void
InitEntity(entity *Entity,
           world_position Position,
           v2f32 Scale,
           rotation_2d Rotation,
           string Name,
           s64 SortingLayer)
{
   Entity->Position = Position;
   Entity->Scale = Scale;
   Entity->Rotation = Rotation;
   SetEntityName(Entity, Name);
   Entity->SortingLayer = SortingLayer;
}

internal void SetCurveControlPoints(entity *Entity, u64 ControlPointCount, local_position *ControlPoints,
                                    f32 *ControlPointWeights, local_position *CubicBezierPoints);

internal void
InitCurve(entity *Entity, curve_params Params)
{
   Entity->Type = Entity_Curve;
   Entity->Curve.CurveParams = Params;
   Entity->Curve.SelectedControlPointIndex = U64_MAX;
   SetCurveControlPoints(Entity, 0, 0, 0, 0);
}

internal void
InitImage(entity *Entity, string ImagePath, sf::Texture *Texture)
{
   Entity->Type = Entity_Image;
   ClearArena(Entity->Arena);
   Entity->Image.ImagePath = StrCopy(Entity->Arena, ImagePath);
   new (&Entity->Image.Texture) sf::Texture(*Texture);
}

internal b32
BeginCurvePoints(curve *Curve, u64 ControlPointCount)
{
   b32 Result = false;
   if (ControlPointCount <= MAX_CONTROL_POINT_COUNT)
   {
      Curve->ControlPointCount = ControlPointCount;
      Result = true;
   }
   
   return Result;
}

internal void
EndCurvePoints(curve *Curve)
{
   Curve->RecomputeRequested = true;
}

#endif //EDITOR_ENTITY_H