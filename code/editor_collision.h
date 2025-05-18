#ifndef EDITOR_COLLISION_H
#define EDITOR_COLLISION_H

typedef u32 collision_flags;
enum
{
 Collision_CurvePoint   = (1<<0),
 Collision_CurveLine    = (1<<1),
 Collision_TrackedPoint = (1<<2),
 Collision_B_SplineKnot = (1<<3),
};
struct collision
{
 entity *Entity;
 collision_flags Flags;
 curve_point_index CurvePointIndex;
 u32 CurveLinePointIndex;
 u32 KnotIndex;
};

struct multiline_collision
{
 b32 Collided;
 u32 PointIndex;
};

internal multiline_collision CheckCollisionWithMultiLine(v2 LocalAtP, v2 *CurveSamples, u32 PointCount, f32 Width, f32 Tolerance);
internal collision CheckCollisionWithEntities(entity_array Entities, v2 AtP, f32 Tolerance);

#endif //EDITOR_COLLISION_H
