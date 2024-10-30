#if EDITOR_PROFILER

internal void
InitProfiler(void)
{
 //GlobalProfiler.CPUFrequency = EstimateCPUFrequency(10);
 GlobalProfiler.StartTSC = ReadCPUTimer();
}

// TODO(hbr): Move it to some different module, probably ui file
#if 0

internal void
FrameProfilePoint(b32 *ViewProfilerWindow)
{
 // NOTE(hbr): End current timing
 GlobalProfiler.EndTSC = ReadCPUTimer();
 u64 ElapsedTotalTSC = GlobalProfiler.EndTSC - GlobalProfiler.StartTSC;
 
 // NOTE(hbr): Render profiler window
 if (*ViewProfilerWindow)
 {      
  bool ViewProfilerWindowAsBool = Cast(bool)*ViewProfilerWindow;
  ImGui::Begin("Profiler", &ViewProfilerWindowAsBool);
  *ViewProfilerWindow = Cast(b32)ViewProfilerWindowAsBool;
  
  if (*ViewProfilerWindow)
  {
   f64 ElapsedTotalMs = 1000.0 * ElapsedTotalTSC / GlobalProfiler.CPUFrequency;
   ImGui::Text("Frame time: %.3fms (%llucy)", ElapsedTotalMs, ElapsedTotalTSC);
   
   for (u64 AnchorIndex = 0;
        AnchorIndex < ArrayCount(GlobalProfiler.Anchors);
        ++AnchorIndex)
   {
    profile_anchor *Anchor = GlobalProfiler.Anchors + AnchorIndex;
    
    if (Anchor->TotalTSC)
    {
     char Buffer[1024];
     int Left = ArrayCount(Buffer);
     int Length = 0;
     
     {
      f64 TotalSelfTSCPercent = 100.0 * Cast(f64)Anchor->TotalSelfTSC / ElapsedTotalTSC;
      int Result = snprintf(Buffer + Length,
                            Left,
                            "%-50s: %10llucy, %10llucy self, %5llu hits, %10llucy/h, %03.2lf%%",
                            Anchor->Label, Anchor->TotalTSC, Anchor->TotalSelfTSC, Anchor->HitCount,
                            Anchor->TotalTSC / Anchor->HitCount, TotalSelfTSCPercent);
      int Written = Min(Left, Result);
      Left -= Written;
      Length += Written;
     }
     
     if (Anchor->TotalTSC != Anchor->TotalSelfTSC)
     {
      f64 TotalTSCPercent = 100.0 * Cast(f64)Anchor->TotalTSC / ElapsedTotalTSC;
      int Result = snprintf(Buffer + Length, Left, ", %3.2f%% w/children", TotalTSCPercent);
      int Written = Min(Left, Result);
      Left -= Written;
      Length += Written;
     }
     
     Buffer[ArrayCount(Buffer) - 1] = 0;
     ImGui::Text(Buffer);
    }
   }
  }
  
  ImGui::End();
 }
 
 // NOTE(hbr): Start new timing
 MemoryZero(GlobalProfiler.Anchors, SizeOf(GlobalProfiler.Anchors));
 GlobalProfiler.StartTSC = ReadCPUTimer();
}

#endif

struct anchor_child
{
 anchor_child *Next;
 u64 ChildIndex;
};

struct anchor_children
{
 anchor_child *Head;
 anchor_child *Tail;
};

internal void
PrintProfileAnchor(profile_anchor *Anchor, u64 TotalElapsedTSC,
                   u64 SpaceAlignment, char *SpaceAlignmentString)
{
 if (Anchor->TotalTSC)
 {
  // TODO(hbr): Don't use this Buffer
  char Buffer[1024];
  int Left = ArrayCount(Buffer);
  int Length = 0;
  
  // TODO(hbr): This code is copied form above. Refactor it
  {
   f64 TotalSelfTSCPercent = 100.0 * Cast(f64)Anchor->TotalSelfTSC / TotalElapsedTSC;
   int Result = Fmt(Buffer + Length, Left,
                    "%-50s: %10llucy, %10llucy self, %5llu hits, %10llucy/h, %03.2lf%%",
                    Anchor->Label, Anchor->TotalTSC, Anchor->TotalSelfTSC, Anchor->HitCount,
                    Anchor->TotalTSC / Anchor->HitCount, TotalSelfTSCPercent);
   int Written = Min(Left, Result);
   Left -= Written;
   Length += Written;
  }
  
  if (Anchor->TotalTSC != Anchor->TotalSelfTSC)
  {
   f64 TotalTSCPercent = 100.0 * Cast(f64)Anchor->TotalTSC / TotalElapsedTSC;
   int Result = Fmt(Buffer + Length, Left, ", %3.2f%% w/children", TotalTSCPercent);
   int Written = Min(Left, Result);
   Left -= Written;
   Length += Written;
  }
  
  Buffer[ArrayCount(Buffer) - 1] = 0;
  Output(StrFromCStr(Buffer));
  Output(StrLit("\n"));
 }
}

internal void
PrintProfileTree(u64 AnchorIndex, anchor_children *Childrens,
                 u64 TotalElapsedTSC, u64 SpaceAlignment,
                 u64 SpaceAlignmentPerLevel, char *SpaceAlignmentString)
{
 profile_anchor *Anchor = GlobalProfiler.Anchors + AnchorIndex;
 u64 NextSpaceAlignment = 0;
 if (Anchor->TotalTSC)
 {
  PrintProfileAnchor(Anchor, TotalElapsedTSC, SpaceAlignment, SpaceAlignmentString);
  NextSpaceAlignment = SpaceAlignment + SpaceAlignmentPerLevel;
 }
 else
 {
  NextSpaceAlignment = SpaceAlignment;
 }
 
 anchor_children *Children = Childrens + AnchorIndex;
 ListIter(Child, Children->Head, anchor_child)
 {
  PrintProfileTree(Child->ChildIndex,
                   Children,
                   TotalElapsedTSC,
                   NextSpaceAlignment,
                   SpaceAlignmentPerLevel,
                   SpaceAlignmentString);
 }
}

internal void
EndProfiling(void)
{
 GlobalProfiler.EndTSC = ReadCPUTimer();
}

internal void
PrintProfiler(void)
{
 temp_arena Temp = TempArena(0);
 
 anchor_children *Childrens = PushArray(Temp.Arena,
                                        ArrayCount(GlobalProfiler.Anchors),
                                        anchor_children);
 for (u64 AnchorIndex = 0;
      AnchorIndex < ArrayCount(GlobalProfiler.Anchors);
      ++AnchorIndex)
 {
  profile_anchor *Anchor = GlobalProfiler.Anchors + AnchorIndex;
  if (Anchor->TotalTSC)
  {
   anchor_children *Children = Childrens + Anchor->ParentAnchorIndex;
   anchor_child *Child = PushStructNonZero(Temp.Arena, anchor_child);
   Child->ChildIndex = AnchorIndex;
   QueuePush(Children->Head, Children->Tail, Child);
  }
 }
 
 u64 TotalElapsedTSC = GlobalProfiler.EndTSC - GlobalProfiler.StartTSC;
 u64 SpaceAlignmentPerLevel = 3;
 u64 SpaceAlignmentStringLength = SpaceAlignmentPerLevel * ArrayCount(GlobalProfiler.Anchors) + 10;
 char *SpaceAlignmentString = PushArray(Temp.Arena, SpaceAlignmentStringLength, char);
 MemorySet(SpaceAlignmentString, ' ', SpaceAlignmentStringLength);
 PrintProfileTree(0, Childrens, TotalElapsedTSC, 0, SpaceAlignmentPerLevel, SpaceAlignmentString);
 
 EndTemp(Temp);
}

#endif