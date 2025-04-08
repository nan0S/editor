#ifndef EDITOR_PROFILER_H
#define EDITOR_PROFILER_H

#define EDITOR_PROFILER 1

#if EDITOR_PROFILER

# define MAX_PROFILER_FRAME_COUNT 512
# define MAX_PROFILER_ANCHOR_COUNT 1024
# define EXPECTED_MAX_CHARS_PER_LABEL 50
# define MAX_PROFILER_LABEL_BUFFER_LENGTH (MAX_PROFILER_ANCHOR_COUNT * EXPECTED_MAX_CHARS_PER_LABEL)

#else

# define MAX_PROFILER_FRAME_COUNT 1
# define MAX_PROFILER_ANCHOR_COUNT 1
# define EXPECTED_MAX_CHARS_PER_LABEL 1
# define MAX_PROFILER_LABEL_BUFFER_LENGTH (MAX_PROFILER_ANCHOR_COUNT * EXPECTED_MAX_CHARS_PER_LABEL)

#endif

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
 profile_anchor Anchors[MAX_PROFILER_ANCHOR_COUNT];
 u64 TotalTSC;
};

struct profiler
{
 anchor_index AnchorParentIndex;
 
 u32 FrameIndex;
 profiler_frame Frames[MAX_PROFILER_FRAME_COUNT];
 
 u16 UsedBlockCount;
 profile_block Blocks[MAX_PROFILER_ANCHOR_COUNT];
 
 u32 LabelBufferAt;
 char LabelBuffer[MAX_PROFILER_LABEL_BUFFER_LENGTH];
 
 string Labels[MAX_PROFILER_ANCHOR_COUNT];
 
 f32 Inv_CPU_Freq;
};
StaticAssert(ArrayCount(MemberOf(profiler_frame, Anchors)) <= MaxUnsignedRepresentableForType(anchor_index),
             ProfilerAnchorIndexIsBigEnough);

#if EDITOR_PROFILER

#define ProfileBegin(Label) __ProfileBegin(Label, __COUNTER__ + 1)
#define ProfileEnd() __ProfileEnd()
#define ProfileFunctionBegin() ProfileBegin(__func__)
#define ProfileBlock(Label) DeferBlock(ProfileBegin(Label), ProfileEnd())

internal void            ProfilerInit(profiler *Profiler);
internal void            ProfilerEquip(profiler *Profiler);
internal void            ProfilerBeginFrame(profiler *Profiler);
internal void            ProfilerEndFrame(profiler *Profiler);

#else

#define ProfileBegin(...)
#define ProfileEnd(...)
#define ProfileFunctionBegin(...)
#define ProfileBlock(...)

#define ProfilerInit(...)
#define ProfilerEquip(...)
#define ProfilerBeginFrame(...)
#define ProfilerEndFrame(...)

#endif

// NOTE(hbr): Don't look at this, just forward declaration for things to compile more independently
internal void __ProfileBegin(char const *Label, u16 AnchorIndex);
internal void __ProfileEnd(void);

#endif //EDITOR_PROFILER_H
