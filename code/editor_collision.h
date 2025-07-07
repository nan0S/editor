#ifndef EDITOR_COLLISION_H
#define EDITOR_COLLISION_H

typedef u32 collision_flags;
enum
{
 Collision_CurvePoint   = (1<<0),
 Collision_CurveLine    = (1<<1),
 Collision_TrackedPoint = (1<<2),
 Collision_BSplineKnot = (1<<3),
};
struct collision
{
 entity *Entity;
 collision_flags Flags;
 curve_point_handle CurvePoint;
 u32 CurveSampleIndex;
 b_spline_knot_handle BSplineKnot;
};

struct multiline_collision
{
 b32 Collided;
 u32 PointIndex;
};

internal collision CheckCollisionWithEntities(entity_array Entities, v2 AtP, f32 Tolerance);

#endif //EDITOR_COLLISION_H
