#ifndef EDITOR_ENTITY_H
#define EDITOR_ENTITY_H

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
CurveRotateAround(entity *CurveEntity, v2 Center, v2 Rotation)
{
 CurveEntity->Rotation = CombineRotations2D(CurveEntity->Rotation, Rotation);
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
 int Cmp1 = Cmp(A->Entity->SortingLayer, B->Entity->SortingLayer);
 if (Cmp1 == 0)
 {
  int Cmp2 = Cmp(A->OriginalOrder, B->OriginalOrder);
  Result = Cmp2;
 }
 else
 {
  Result = Cmp1;
 }
 
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
           v2 P,
           v2 Scale,
           v2 Rotation,
           string Name,
           s64 SortingLayer)
{
 Entity->P = P;
 Entity->Scale = Scale;
 Entity->Rotation = Rotation;
 SetEntityName(Entity, Name);
 Entity->SortingLayer = SortingLayer;
}

internal void SetCurveControlPoints(entity *Entity, u64 ControlPointCount, v2 *ControlPoints,
                                    f32 *ControlPointWeights, cubic_bezier_point *CubicBezierPoints);

// TODO(hbr): remove this
internal void UnselectControlPoint(curve *Curve);

internal void
InitCurve(entity *Entity, curve_params Params)
{
 Entity->Type = Entity_Curve;
 Entity->Curve.CurveParams = Params;
 UnselectControlPoint(&Entity->Curve);
 SetCurveControlPoints(Entity, 0, 0, 0, 0);
}

internal void
InitImage(entity *Entity)
{
 Entity->Type = Entity_Image;
 ClearArena(Entity->Arena);
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

internal v2
WorldToLocalEntityPosition(entity *Entity, v2 P)
{
 v2 Result = RotateAround(P - Entity->P,
                          V2(0.0f, 0.0f),
                          Rotation2DInverse(Entity->Rotation));
 return Result;
}

// TODO(hbr): Rename this function into MarkCurveChanged or sth like that
internal void
RecomputeCurve(entity *Entity)
{
 Entity->Curve.RecomputeRequested = true;
}

// TODO(hbr): remove this
internal void CalculateBezierCubicPointAt(u64 N, v2 *P, cubic_bezier_point *Out, u64 At);

#endif //EDITOR_ENTITY_H