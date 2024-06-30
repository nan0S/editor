#ifndef BUILD_DEBUG_H
#define BUILD_DEBUG_H

// TODO(hbr): Refactor to return some kind of struct, not keep calculations inside frame_stats struct
struct frame_stats
{
   struct {
      u64 FrameCount;
      f32 MinFrameTime;
      f32 MaxFrameTime;
      f32 SumFrameTime;
   } Calculation;
   
   f32 FPS;
   f32 MinFrameTime;
   f32 MaxFrameTime;
   f32 AvgFrameTime;
};

// TODO(hbr): get rid of this ugly forward declaration
struct editor;

internal frame_stats CreateFrameStats(void);
internal void        FrameStatsUpdate(frame_stats *Stats, f32 FrameTime);

internal void DebugUpdateAndRender(editor *Editor);

#endif //BUILD_DEBUG_H
