#ifndef EDITOR_DEBUG_H
#define EDITOR_DEBUG_H

// TODO(hbr): Move from editor_debug.h
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

function frame_stats CreateFrameStats(void);
function void        FrameStatsUpdate(frame_stats *Stats, f32 FrameTime);

function void DebugUpdateAndRender(editor *Editor);

#endif //EDITOR_DEBUG_H
