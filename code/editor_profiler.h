#ifndef EDITOR_PROFILER_H
#define EDITOR_PROFILER_H

// TODO(hbr): Revise this whole module

#if !defined(EDITOR_PROFILER)
# define EDITOR_PROFILER 0
#endif

#if EDITOR_PROFILER

struct profile_anchor
{
 u64 TotalTSC;
 u64 TotalSelfTSC;
 u64 HitCount;
 char const *Label;
 // TODO(hbr): Maybe make it smaller than u64, maybe get rid of it entirely
 u64 ParentAnchorIndex;
};

struct profiler
{
 profile_anchor Anchors[1024];
 u64 CPUFrequency;
 u64 StartTSC;
 u64 EndTSC;
};
global profiler GlobalProfiler;
global u64 GlobalAnchorParentIndex;

struct profile_block
{
 profile_block(u64 AnchorIndex_, char const *Label_)
 {
  profile_anchor *Anchor = GlobalProfiler.Anchors + AnchorIndex_;
  
  AnchorIndex = AnchorIndex_;
  Label = Label_;
  ParentAnchorIndex = GlobalAnchorParentIndex;
  OldTotalTSC = Anchor->TotalTSC;
  GlobalAnchorParentIndex = AnchorIndex_;
  
  StartTSC = ReadCPUTimer();
 }
 
 ~profile_block()
 {
  u64 ElapsedTSC = ReadCPUTimer() - StartTSC;
  
  profile_anchor *Anchor = GlobalProfiler.Anchors + AnchorIndex;
  profile_anchor *Parent = GlobalProfiler.Anchors + ParentAnchorIndex;
  
  Anchor->TotalTSC = OldTotalTSC + ElapsedTSC;
  Anchor->TotalSelfTSC += ElapsedTSC;
  Anchor->HitCount += 1;
  Anchor->Label = Label;
  Anchor->ParentAnchorIndex = ParentAnchorIndex;
  Parent->TotalSelfTSC -= ElapsedTSC;
  
  GlobalAnchorParentIndex = ParentAnchorIndex;
 }
 
 u64 AnchorIndex;
 char const *Label;
 u64 StartTSC;
 u64 ParentAnchorIndex;
 u64 OldTotalTSC;
};

internal void InitProfiler(void);
internal void EndProfiling(void);
internal void PrintProfiler(void);

internal void FrameProfilePoint(b32 *ViewProfilerWindow);

#define TimeBlock(Label) profile_block NameConcat(ProfileBlock, __LINE__)(__COUNTER__ + 1, Label)

#else

#define InitProfiler(...)
#define EndProfiling(...)
#define PrintProfiler(...)
#define FrameProfilePoint(...)
#define TimeBlock(...)

#endif

#define TimeFunction TimeBlock(__func__)

#endif //EDITOR_PROFILER_H
