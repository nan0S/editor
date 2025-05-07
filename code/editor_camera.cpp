internal void
TranslateCamera(camera *Camera, v2 Translate)
{
 Camera->P += Translate;
 Camera->ReachingTarget = false;
}

internal void
SetCameraTarget(camera *Camera, rect2 AABB, f32 AspectRatio)
{
 if (IsNonEmpty(&AABB))
 {
  v2 TargetP = 0.5f * (AABB.Min + AABB.Max);
  Camera->TargetP = TargetP;
  
#if 0
  v2 Extents = AABB.Max - TargetP;
  Camera->TargetZoom = Min(SafeDiv1(1.0f, Extents.X), SafeDiv1(1.0f, Extents.Y));
#endif
  
  {
   mat3_inv CameraT = CameraTransform(TargetP, Camera->Rotation, 1.0f);
   mat3_inv ClipT = ClipTransform(AspectRatio);
   
   v2 MaxP = V2(1.0f, 1.0f);
   
   v2 P = ClipT.Forward * CameraT.Forward * MaxP;
   
   f32 ZoomX = 1.0f / P.X;
   f32 ZoomY = 1.0f / P.Y;
   
   f32 NewZoom = Min(ZoomX, ZoomY);
   Camera->TargetZoom = NewZoom;
  }
  
  
  Camera->ReachingTarget = true;
 }
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
UpdateCamera(camera *Camera, platform_input_ouput *Input)
{
 if (Camera->ReachingTarget)
 {
  f32 t = 1.0f - PowF32(2.0f, -Camera->ReachingTargetSpeed * Input->dtForFrame);
  Camera->P = Lerp(Camera->P, Camera->TargetP, t);
  Camera->Zoom = Lerp(Camera->Zoom, Camera->TargetZoom, t);
  
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
