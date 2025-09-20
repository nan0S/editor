/* ========================================================================
   Parametric Curve Editor
   Master's Thesis by Hubert Obrzut
   Supervisor: Paweł Woźny
   University of Wrocław
   Faculty of Mathematics and Computer Science
   Institute of Computer Science
   Date: September 2025
   ======================================================================== */

#ifndef EDITOR_PROFILER_H
#define EDITOR_PROFILER_H

#define EDITOR_PROFILER BUILD_DEV

#if EDITOR_PROFILER

# define MAX_PROFILER_FRAME_COUNT 1024
# define MAX_PROFILER_ANCHOR_COUNT 1024
# define EXPECTED_MAX_CHARS_PER_LABEL 50
# define MAX_PROFILER_LABEL_BUFFER_LENGTH (MAX_PROFILER_ANCHOR_COUNT * EXPECTED_MAX_CHARS_PER_LABEL)

# define COMPILATION_UNIT_PROFILER_ANCHOR_INDEX_OFFSET (COMPILATION_UNIT_INDEX * MAX_PROFILER_ANCHOR_COUNT / COMPILATION_UNIT_COUNT)
# define COMPILATION_UNIT_PROFILER_MAX_ANCHOR_COUNT (MAX_PROFILER_ANCHOR_COUNT / COMPILATION_UNIT_COUNT)

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
 char const *File;
 int Line;
 u64 StartTSC;
};

struct profiler_frame
{
 profile_anchor Anchors[MAX_PROFILER_ANCHOR_COUNT];
 u64 TotalTSC;
};

struct profile_anchor_source_code_location
{
 char const *File;
 u32 Line;
};

struct profiler
{
 anchor_index AnchorParentIndex;
 
 u32 FrameIndex;
 profiler_frame Frames[MAX_PROFILER_FRAME_COUNT];
 profiler_frame NilFrame;
 
 profiler_frame *CurrentFrame;
 
 u16 UsedBlockCount;
 profile_block Blocks[MAX_PROFILER_ANCHOR_COUNT];
 
 u32 LabelBufferAt;
 char LabelBuffer[MAX_PROFILER_LABEL_BUFFER_LENGTH];
 string AnchorLabels[MAX_PROFILER_ANCHOR_COUNT];
 
 profile_anchor_source_code_location
  AnchorLocations[MAX_PROFILER_ANCHOR_COUNT];
 
 f32 Inv_CPU_Freq;
};
StaticAssert(ArrayCount(MemberOf(profiler_frame, Anchors)) <= MaxUnsignedRepresentableForType(anchor_index),
             ProfilerAnchorIndexIsBigEnough);

#if EDITOR_PROFILER

#define ProfileBegin(Label) __ProfileBegin(Label, __FILE__, __LINE__, __COUNTER__ + 1 + COMPILATION_UNIT_PROFILER_ANCHOR_INDEX_OFFSET)
#define ProfileEnd() __ProfileEnd()
#define ProfileFunctionBegin() ProfileBegin(__func__)
#define ProfileBlock(Label) DeferBlock(ProfileBegin(Label), ProfileEnd())

internal void ProfilerInit(profiler *Profiler);
internal void ProfilerEquip(profiler *Profiler);
internal void ProfilerBeginFrame(profiler *Profiler);
internal void ProfilerEndFrame(profiler *Profiler);
internal void ProfilerReset(profiler *Profiler); // useful when some code hot-reloaded because anchor indices might be stale

#else

#define ProfileBegin(...)
#define ProfileEnd(...)
#define ProfileFunctionBegin(...)
#define ProfileBlock(...)

#define ProfilerInit(...)
#define ProfilerEquip(...)
#define ProfilerBeginFrame(...)
#define ProfilerEndFrame(...)
#define ProfilerReset(...)

#endif

internal b32          ProfilerIsAnchorActive(anchor_index Index);
internal anchor_index MakeAnchorIndex(u32 Index);

#endif //EDITOR_PROFILER_H
