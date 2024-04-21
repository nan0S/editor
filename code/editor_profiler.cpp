#if EDITOR_PROFILER

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

function void
InitializeProfiler(void)
{
   GlobalProfiler.CPUFrequency = EstimateCPUFrequency(10);
}

function void
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
                  int Written = Minimum(Left, Result);
                  Left -= Written;
                  Length += Written;
               }
               
               if (Anchor->TotalTSC != Anchor->TotalSelfTSC)
               {
                  f64 TotalTSCPercent = 100.0 * Cast(f64)Anchor->TotalTSC / ElapsedTotalTSC;
                  int Result = snprintf(Buffer + Length, Left, ", %3.2f%% w/children", TotalTSCPercent);
                  int Written = Minimum(Left, Result);
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

// TODO(hbr): Remove
#if 0

function void
BeginProfile(void)
{
   GlobalProfiler.StartTSC = ReadCPUTimer();
}

internal void
PrintProfileAnchor(profile_anchor *Anchor, u64 TotalElapsedTSC,
                   u64 SpaceAlignment, char *SpaceAlignmentString)
{
   if (Anchor->TotalElapsedTSC)
   {
      u64 AnchorTotalElapsedTSC = Anchor->TotalElapsedTSC - Anchor->TotalElapsedChildrenTSC;
      f64 Percent = 100.0 * Cast(f64)AnchorTotalElapsedTSC / TotalElapsedTSC;
      
      char Temp = SpaceAlignmentString[SpaceAlignment];
      SpaceAlignmentString[SpaceAlignment] = '\0';
      
      printf("%s%s[%llu]: %llu (%.2f%%",
             SpaceAlignmentString, Anchor->Label, Anchor->HitCount,
             Anchor->TotalElapsedTSC, Percent);
      
      SpaceAlignmentString[SpaceAlignment] = Temp;
      
      if (Anchor->TotalElapsedChildrenTSC)
      {
         f64 PercentWithChildren = 100.0 * Cast(f64)Anchor->TotalElapsedTSC / TotalElapsedTSC;
         printf(", %.2f%% w/children", PercentWithChildren);
      }
      printf(")\n");
   }
}

internal void
PrintProfileTree(u64 AnchorIndex, anchor_children *AnchorsChildren,
                 u64 TotalElapsedTSC, u64 SpaceAlignment,
                 u64 SpaceAlignmentPerLevel, char *SpaceAlignmentString)
{
   profile_anchor *CurrentAnchor = &GlobalProfiler.Anchors[AnchorIndex];
   u64 NextSpaceAlignment = 0;
   if (CurrentAnchor->TotalElapsedTSC)
   {
      PrintProfileAnchor(CurrentAnchor,
                         TotalElapsedTSC, SpaceAlignment,
                         SpaceAlignmentString);
      
      NextSpaceAlignment = SpaceAlignment + SpaceAlignmentPerLevel;
   }
   else
   {
      NextSpaceAlignment = SpaceAlignment;
   }
   
   anchor_children Children = AnchorsChildren[AnchorIndex];
   ListIter(Child, Children.Head, anchor_child)
   {
      PrintProfileTree(Child->ChildIndex,
                       AnchorsChildren,
                       TotalElapsedTSC,
                       NextSpaceAlignment,
                       SpaceAlignmentPerLevel,
                       SpaceAlignmentString);
   }
}

function void
EndAndPrintProfile(void)
{
   u64 EndTSC = ReadCPUTimer();
   
   temp_arena Temp = TempArena(0);
   defer { EndTemp(Temp); };
   
   anchor_children *AnchorsChildren = PushArray(Temp.Arena,
                                                ArrayCount(GlobalProfiler.Anchors),
                                                anchor_children);
   
   for (u64 AnchorIndex = 0;
        AnchorIndex < ArrayCount(GlobalProfiler.Anchors);
        ++AnchorIndex)
   {
      profile_anchor *Anchor = &GlobalProfiler.Anchors[AnchorIndex];
      
      if (Anchor->TotalElapsedTSC)
      {
         anchor_children *AnchorChildren = &AnchorsChildren[Anchor->ParentAnchorIndex];
         
         anchor_child *NewChild = PushStructNonZero(Temp.Arena, anchor_child);
         NewChild->ChildIndex = AnchorIndex;
         
         QueuePush(AnchorChildren->Head,
                   AnchorChildren->Tail,
                   NewChild);
      }
   }
   
   u64 TotalElapsedTSC = EndTSC - GlobalProfiler.StartTSC;
   
   u64 SpaceAlignmentPerLevel = 3;
   u64 SpaceAlignmentStringLength = SpaceAlignmentPerLevel * ArrayCount(GlobalProfiler.Anchors) + 10;
   char *SpaceAlignmentString = PushArray(Temp.Arena, SpaceAlignmentStringLength, char);
   MemorySet(SpaceAlignmentString, ' ', SpaceAlignmentStringLength);
   
   PrintProfileTree(0, AnchorsChildren, TotalElapsedTSC, 0,
                    SpaceAlignmentPerLevel, SpaceAlignmentString);
}

#endif

#endif