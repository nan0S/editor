global profiler *GlobalProfiler;

internal void
ProfilerInit(profiler *Profiler)
{
 u64 CPU_Freq = OS_CPUTimerFreq();
 Profiler->Inv_CPU_Freq = SafeDiv0(1.0f, CPU_Freq);
}

internal void
ProfilerEquip(profiler *Profiler)
{
 GlobalProfiler = Profiler;
}

internal void
ProfilerBeginFrame(profiler *Profiler)
{
 ++Profiler->FrameIndex;
 if (Profiler->FrameIndex == MAX_PROFILER_FRAME_COUNT)
 {
  Profiler->FrameIndex = 0;
 }
 
 profiler_frame *Frame = ProfilerCurrentFrame(Profiler);
 ArrayZero(Frame->Anchors, MAX_PROFILER_ANCHOR_COUNT);
 
 Frame->TotalTSC = (0 - OS_ReadCPUTimer());
}

internal void
ProfilerEndFrame(profiler *Profiler)
{
 u64 TSC = OS_ReadCPUTimer();
 profiler_frame *Frame = ProfilerCurrentFrame(Profiler);
 Frame->TotalTSC += TSC;
}

inline internal profiler_frame *
ProfilerCurrentFrame(profiler *Profiler)
{
 profiler_frame *Frame = Profiler->Frames + Profiler->FrameIndex;
 return Frame;
}

inline internal void
__ProfileBegin(char const *Label, u16 AnchorIndex)
{
 profiler *Profiler = GlobalProfiler;
 profiler_frame *Frame = ProfilerCurrentFrame(Profiler);
 
 Assert(Profiler->UsedBlockCount < ArrayCount(Profiler->Blocks));
 profile_block *Block = Profiler->Blocks + Profiler->UsedBlockCount++;
 
 profile_anchor *Anchor = Frame->Anchors + AnchorIndex;
 
 Block->OldTotalTSC = Anchor->TotalTSC;
 Block->AnchorIndex.Index = AnchorIndex;
 Block->ParentIndex = Profiler->AnchorParentIndex;
 Profiler->AnchorParentIndex.Index = AnchorIndex;
 Block->Label = Label;
 
 Block->StartTSC = OS_ReadCPUTimer();
}

inline internal void
__ProfileEnd(void)
{
 profiler *Profiler = GlobalProfiler;
 profiler_frame *Frame = ProfilerCurrentFrame(Profiler);
 
 Assert(Profiler->UsedBlockCount > 0);
 profile_block *Block = Profiler->Blocks + --Profiler->UsedBlockCount;
 
 u64 ElapsedTSC = OS_ReadCPUTimer() - Block->StartTSC;
 
 profile_anchor *Anchor = Frame->Anchors + Block->AnchorIndex.Index;
 profile_anchor *Parent = Frame->Anchors + Block->ParentIndex.Index;
 
 Anchor->TotalTSC = Block->OldTotalTSC + ElapsedTSC;
 Anchor->TotalSelfTSC += ElapsedTSC;
 ++Anchor->HitCount;
 Anchor->Parent = Block->ParentIndex;
 Anchor->Label = Block->Label;
 Parent->TotalSelfTSC -= ElapsedTSC;
 
 Profiler->AnchorParentIndex = Block->ParentIndex;
}