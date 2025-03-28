#if EDITOR_PROFILER

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

inline internal profiler_frame *
ProfilerCurrentFrame(profiler *Profiler)
{
 profiler_frame *Frame = Profiler->Frames + Profiler->FrameIndex;
 return Frame;
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
 u64 EndTSC = OS_ReadCPUTimer();
 
 profiler *Profiler = GlobalProfiler;
 profiler_frame *Frame = ProfilerCurrentFrame(Profiler);
 
 Assert(Profiler->UsedBlockCount > 0);
 profile_block *Block = Profiler->Blocks + --Profiler->UsedBlockCount;
 
 anchor_index AnchorIndex = Block->AnchorIndex;
 
 profile_anchor *Anchor = Frame->Anchors + AnchorIndex.Index;
 profile_anchor *Parent = Frame->Anchors + Block->ParentIndex.Index;
 
 u64 ElapsedTSC = EndTSC - Block->StartTSC;
 
 Anchor->TotalTSC = Block->OldTotalTSC + ElapsedTSC;
 Anchor->TotalSelfTSC += ElapsedTSC;
 ++Anchor->HitCount;
 Anchor->Parent = Block->ParentIndex;
 Parent->TotalSelfTSC -= ElapsedTSC;
 
 if (Profiler->Labels[AnchorIndex.Index].Data == 0)
 {
  u32 LabelBufferAt = Profiler->LabelBufferAt;
  u32 SpaceLeft = MAX_PROFILER_LABEL_BUFFER_LENGTH - LabelBufferAt;
  
  char *Label = Profiler->LabelBuffer + LabelBufferAt;
  u32 LabelLength = SafeCastU32(CStrLen(Block->Label));
  MemoryCopy(Label, Block->Label, LabelLength);
  
  Profiler->LabelBufferAt = LabelBufferAt + LabelLength;
  Profiler->Labels[AnchorIndex.Index] = MakeStr(Label, LabelLength);
 }
 
 Profiler->AnchorParentIndex = Block->ParentIndex;
}

#endif