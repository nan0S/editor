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

internal void InitCamera(camera *Camera);
internal void TranslateCamera(camera *Camera, v2 Translate);
internal void SetCameraTarget(camera *Camera, rect2 AABB, f32 AspectRatio);
internal void SetCameraZoom(camera *Camera, f32 Zoom);
internal void RotateCameraAround(camera *Camera, v2 Rotate, v2 Around);
internal void ZoomCamera(camera *Camera, f32 By);
internal void UpdateCamera(camera *Camera, platform_input_output *Input);

#endif //EDITOR_CAMERA_H
