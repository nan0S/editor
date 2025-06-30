#ifndef EDITOR_CAMERA_H
#define EDITOR_CAMERA_H

struct exponential_animation
{
 f32 PowBase;
 f32 ExponentMult;
};

struct camera
{
 v2 P;
 rotation2d Rotation;
 f32 Zoom;
 f32 ZoomSensitivity;
 
 b32 ReachingTarget;
 exponential_animation Animation;
 f32 ReachingTargetSpeed;
 v2 TargetP;
 f32 TargetZoom;
};

internal void InitCamera(camera *Camera);
internal void TranslateCamera(camera *Camera, v2 Translate);
internal void SetCameraTarget(camera *Camera, v2 TargetP, f32 TargetZoom);
internal void SetCameraZoom(camera *Camera, f32 Zoom);
internal void RotateCameraAround(camera *Camera, v2 Rotate, v2 Around);
internal void ZoomCamera(camera *Camera, f32 By);
internal void UpdateCamera(camera *Camera, platform_input_output *Input);

internal exponential_animation MakeExponentialAnimation(f32 PowBase, f32 ExponentMult);
internal exponential_animation ExponentialAnimationDefault(void);
internal f32 EvaluateAnimation(exponential_animation *Anim, f32 dt);

#endif //EDITOR_CAMERA_H
