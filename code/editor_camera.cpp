internal void
InitCamera(camera *Camera)
{
 Camera->P = V2(0.0f, 0.0f);
 Camera->Rotation = Rotation2DZero();
 Camera->Zoom = 1.0f;
 Camera->ZoomSensitivity = 0.05f;
 Camera->ReachingTargetSpeed = 1.0f;
 // NOTE(hbr): Parameters chosen experimentally
 Camera->Animation = MakeExponentialAnimation(5.2f, 6.6f);
}

internal void
TranslateCamera(camera *Camera, v2 Translate)
{
 Camera->P += Translate;
 Camera->ReachingTarget = false;
}

internal void
SetCameraTarget(camera *Camera, v2 TargetP, f32 TargetZoom)
{
 Camera->TargetP = TargetP;
 Camera->TargetZoom = TargetZoom;
 Camera->ReachingTarget = true;
}

internal void
SetCameraZoom(camera *Camera, f32 Zoom)
{
 Camera->Zoom = Zoom;
 Camera->ReachingTarget = false;
}

internal void
RotateCameraAround(camera *Camera, v2 Rotate, v2 Around)
{
 Camera->P = RotateAround(Camera->P, Around, Rotate);
 Camera->Rotation = CombineRotations2D(Camera->Rotation, Rotate);
 Camera->ReachingTarget = false;
}

internal void
ZoomCamera(camera *Camera, f32 By)
{
 f32 ZoomDelta = Camera->Zoom * Camera->ZoomSensitivity * By;
 Camera->Zoom += ZoomDelta;
 Camera->ReachingTarget = false;
}

internal void
UpdateCamera(camera *Camera, platform_input_output *Input)
{
 if (Camera->ReachingTarget)
 {
  f32 dt = UseAndExtractDeltaTime(Input);
  f32 t = EvaluateAnimation(&Camera->Animation, dt);
  Camera->P = Lerp(Camera->P, Camera->TargetP, t);
  // NOTE(hbr): Zooming slower than moving the camera looks way more natural
  Camera->Zoom = Lerp(Camera->Zoom, Camera->TargetZoom, t/4);
  
  if (ApproxEq32(Camera->Zoom, Camera->TargetZoom) &&
      ApproxEq32(Camera->P.X, Camera->TargetP.X) &&
      ApproxEq32(Camera->P.Y, Camera->TargetP.Y))
  {
   Camera->P = Camera->TargetP;
   Camera->Zoom = Camera->TargetZoom;
   Camera->ReachingTarget = false;
  }
 }
}

internal exponential_animation
MakeExponentialAnimation(f32 PowBase, f32 ExponentMult)
{
 exponential_animation Anim = {};
 Anim.PowBase = PowBase;
 Anim.ExponentMult = ExponentMult;
 return Anim;
}

internal exponential_animation
ExponentialAnimationDefault(void)
{
 exponential_animation Anim = MakeExponentialAnimation(2.0f, 4.0f);
 return Anim;
}

inline internal f32
EvaluateAnimation(exponential_animation *Anim, f32 dt)
{
 f32 T = 1.0f - PowF32(Anim->PowBase, -Anim->ExponentMult * dt);
 return T;
}
