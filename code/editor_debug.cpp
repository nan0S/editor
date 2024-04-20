function frame_stats
FrameStatsMake(void)
{
   frame_stats Result = {};
   Result.Calculation.MinFrameTime = INF_F32;
   Result.Calculation.MaxFrameTime = -INF_F32;
   
   return Result;
}

function void
FrameStatsUpdate(frame_stats *Stats, f32 FrameTime)
{
   Stats->Calculation.FrameCount += 1;
   Stats->Calculation.SumFrameTime += FrameTime;
   Stats->Calculation.MinFrameTime = Minimum(Stats->Calculation.MinFrameTime, FrameTime);
   Stats->Calculation.MaxFrameTime = Maximum(Stats->Calculation.MaxFrameTime, FrameTime);
   
   if (Stats->Calculation.SumFrameTime >= 1.0f)
   {
      Stats->FPS = Stats->Calculation.FrameCount / Stats->Calculation.SumFrameTime;
      Stats->MinFrameTime = Stats->Calculation.MinFrameTime;
      Stats->MaxFrameTime = Stats->Calculation.MaxFrameTime;
      Stats->AvgFrameTime = Stats->Calculation.SumFrameTime / Stats->Calculation.FrameCount;
      
      Stats->Calculation.FrameCount = 0;
      Stats->Calculation.MinFrameTime = INF_F32;
      Stats->Calculation.MaxFrameTime = -INF_F32;
      Stats->Calculation.SumFrameTime = 0.0f;
   }
}