#ifndef EDITOR_CAMERA_H
#define EDITOR_CAMERA_H

struct camera
{
 v2 P;
 v2 Rotation;
 f32 Zoom;
 f32 ZoomSensitivity;
 
 b32 ReachingTarget;
 f32 ReachingTargetSpeed;
 v2 TargetP;
 f32 TargetZoom;
};

#endif //EDITOR_CAMERA_H
