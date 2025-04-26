#if EDITOR_PROFILER

global profiler *GlobalProfiler;

internal void
ProfilerInit(profiler *Profiler)
{
 u64 CPU_Freq = OS_CPUTimerFreq();
 Profiler->Inv_CPU_Freq = SafeDiv0(1.0f, CPU_Freq);
 //Profiler->CurrentFrame = &Profiler->NilFrame;
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
 
 profiler_frame *CurrentFrame = Profiler->Frames + Profiler->FrameIndex;
 Profiler->CurrentFrame = CurrentFrame;
 ArrayZero(CurrentFrame->Anchors, MAX_PROFILER_ANCHOR_COUNT);
 
 CurrentFrame->TotalTSC = (0 - OS_ReadCPUTimer());
}

internal void
ProfilerEndFrame(profiler *Profiler)
{
 u64 TSC = OS_ReadCPUTimer();
 Assert(Profiler->UsedBlockCount == 0);
 profiler_frame *Frame = Profiler->CurrentFrame;
 Frame->TotalTSC += TSC;
 Profiler->CurrentFrame = &Profiler->NilFrame;
}

internal void
ProfilerReset(profiler *Profiler)
{
 StructZero(Profiler);
 ProfilerInit(Profiler);
}

inline internal void
__ProfileBegin(char const *Label, u16 AnchorIndex)
{
 Assert(COMPILATION_UNIT_PROFILER_ANCHOR_INDEX_OFFSET <= AnchorIndex &&
        AnchorIndex < (COMPILATION_UNIT_PROFILER_ANCHOR_INDEX_OFFSET +
                       COMPILATION_UNIT_PROFILER_MAX_ANCHOR_COUNT));
 
 profiler *Profiler = GlobalProfiler;
 profiler_frame *Frame = Profiler->CurrentFrame;
 
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
 profiler_frame *Frame = Profiler->CurrentFrame;
 
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
  u32 CopyLength = Min(SpaceLeft, LabelLength);
  MemoryCopy(Label, Block->Label, CopyLength);
  
  Profiler->LabelBufferAt = LabelBufferAt + CopyLength;
  Profiler->Labels[AnchorIndex.Index] = MakeStr(Label, CopyLength);
 }
 
 Profiler->AnchorParentIndex = Block->ParentIndex;
}

#endif