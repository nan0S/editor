/* ========================================================================
   Parametric Curve Editor
   Master's Thesis by Hubert Obrzut
   Supervisor: Paweł Woźny
   University of Wrocław
   Faculty of Mathematics and Computer Science
   Institute of Computer Science
   Date: September 2025
   ======================================================================== */

internal date_time
TimestampToDateTime(timestamp64 Ts)
{
 date_time Dt = {};
 Dt.Ms = Ts % 1000;
 Ts /= 1000;
 Dt.Sec = Ts % 60;
 Ts /= 60;
 Dt.Mins = Ts % 60;
 Ts /= 60;
 Dt.Hour = Ts % 24;
 Ts /= 24;
 Dt.Day = Ts % 31;
 Ts /= 31;
 Dt.Month = Ts % 12;
 Ts /= 12;
 Dt.Year = SafeCastU32(Ts);
 
 return Dt;
}

internal timestamp64
DateTimeToTimestamp(date_time Dt)
{
 timestamp64 Ts = 0;
 Ts += Dt.Year;
 Ts *= 12;
 Ts += Dt.Month;
 Ts *= 31;
 Ts += Dt.Day;
 Ts *= 24;
 Ts += Dt.Hour;
 Ts *= 60;
 Ts += Dt.Mins; 
 Ts *= 60;
 Ts += Dt.Sec;
 Ts *= 1000;
 Ts += Dt.Ms;
 
 return Ts;
}

inline internal operating_system
DetectOS(void)
{
#if OS_WINDOWS
 operating_system OS = OS_Win32;
#endif
#if OS_LINUX
 operating_system OS = OS_Linux;
#endif
 return OS;
}

internal rect2
EmptyAABB(void)
{
 rect2 Result = {};
 Result.Min = V2(F32_INF, F32_INF);
 Result.Max = V2(-F32_INF, -F32_INF);
 
 return Result;
}

internal void
AddPointAABB(rect2 *AABB, v2 P)
{
 if (P.X < AABB->Min.X) AABB->Min.X = P.X;
 if (P.X > AABB->Max.X) AABB->Max.X = P.X;
 if (P.Y < AABB->Min.Y) AABB->Min.Y = P.Y;
 if (P.Y > AABB->Max.Y) AABB->Max.Y = P.Y;
}

internal b32
IsNonEmpty(rect2 *Rect)
{
 b32 Result = ((Rect->Min.X <= Rect->Max.X) &&
               (Rect->Min.Y <= Rect->Max.Y));
 return Result;
}

internal rect2_corners
AABBCorners(rect2 Rect)
{
 rect2_corners Corners = {};
 Corners.Corners[Corner_00] = V2(Rect.Min.X, Rect.Min.Y);
 Corners.Corners[Corner_01] = V2(Rect.Min.X, Rect.Max.Y);
 Corners.Corners[Corner_10] = V2(Rect.Max.X, Rect.Min.Y);
 Corners.Corners[Corner_11] = V2(Rect.Max.X, Rect.Max.Y);
 return Corners;
}