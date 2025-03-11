#ifndef EDITOR_PROFILER_H
#define EDITOR_PROFILER_H

struct label_index
{
 u16 Index;
};
struct anchor_index
{
 u16 Index;
};
struct profile_anchor
{
 u64 TotalTSC;
 u64 TotalSelfTSC;
 u32 HitCount;
 anchor_index Parent;
 char const *Label;
};

struct profile_block
{
 u64 OldTotalTSC;
 anchor_index AnchorIndex;
 anchor_index ParentIndex;
 char const *Label;
 u64 StartTSC;
};

struct profiler_frame
{
#define MAX_PROFILER_ANCHOR_COUNT 1024
 profile_anchor Anchors[MAX_PROFILER_ANCHOR_COUNT];
 u64 TotalTSC;
};

struct profiler
{
 anchor_index AnchorParentIndex;
 
 u32 FrameIndex;
#define MAX_PROFILER_FRAME_COUNT 512
 profiler_frame Frames[MAX_PROFILER_FRAME_COUNT];
 
 u16 UsedBlockCount;
 profile_block Blocks[MAX_PROFILER_ANCHOR_COUNT];
 
 f32 Inv_CPU_Freq;
};
StaticAssert(ArrayCount(MemberOf(profiler_frame, Anchors)) <= ((1 << (SizeOf(anchor_index)*8)) - 1), ProfileAnchorArrayIsBigEnough);

internal void            ProfilerInit(profiler *Profiler);
internal void            ProfilerEquip(profiler *Profiler);
internal void            ProfilerBeginFrame(profiler *Profiler);
internal void            ProfilerEndFrame(profiler *Profiler);
internal profiler_frame *ProfilerCurrentFrame(profiler *Profiler);

#define ProfileBegin(Label) __ProfileBegin(Label, __COUNTER__ + 1)
#define ProfileEnd() __ProfileEnd()
#define ProfileFunctionBegin() ProfileBegin(__func__)
#define ProfileBlock(Label) DeferBlock(ProfileBegin(Label), ProfileEnd())

#endif //EDITOR_PROFILER_H
