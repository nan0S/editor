inline internal sort_entry_array
AllocSortEntryArray(arena *Arena, u32 MaxCount, sort_order Order)
{
 sort_entry_array Result = {};
 Result.MaxCount = MaxCount;
 Result.Entries = PushArrayNonZero(Arena, MaxCount, sort_entry);
 Result.Order = Order;
 
 return Result;
}

internal void
AddSortEntry(sort_entry_array *Array, sort_key_f32 SortKey, u32 Index)
{
 Assert(Array->Count < Array->MaxCount);
 
 sort_entry *Entry = Array->Entries + Array->Count++;
 Entry->SortKey = (Array->Order == SortOrder_Ascending ? SortKey : -SortKey);
 Entry->Index = Index;
}

internal void
ClearSortEntryArray(sort_entry_array *Array)
{
 Array->Count = 0;
}

internal void
QuickSort(sort_entry *Entries, u32 Count)
{
 if (Count > 0)
 { 
  u32 PivotIndex = Count - 1;
  sort_key_f32 Pivot = Entries[PivotIndex].SortKey;
  
  u32 Left = 0;
  for (u32 Index = 0;
       Index < Count;
       ++Index)
  {
   if (Entries[Index].SortKey <= Pivot)
   {
    Swap(Entries[Index], Entries[Left], sort_entry);
    ++Left;
   }
  }
  
  Assert(Left > 0);
  QuickSort(Entries, Left - 1);
  QuickSort(Entries + Left, Count - Left);
 }
}

internal sort_entry *
MergeSortStable(sort_entry *Entries, u32 Count, sort_entry *TempMemory)
{
 sort_entry *Curr = Entries;
 sort_entry *Next = TempMemory;
 
 for (u32 Length = 1;
      Length <= Count;
      Length <<= 1)
 {
  for (u32 Index = 0;
       Index < Count;
       Index += (Length << 1))
  {
   u32 LeftIndex = Index;
   u32 RightIndex = LeftIndex + Length;
   
   sort_entry *LeftAt = Curr + LeftIndex;
   sort_entry *RightAt = Curr + RightIndex;
   
   u32 Left = Min(Count - ClampTop(LeftIndex, Count), Length);
   u32 Right = Min(Count - ClampTop(RightIndex, Count), Length);
   u32 Total = Left + Right;
   sort_entry *NextAt = Next + Index;
   
   while (Total--)
   {
    sort_entry *Less;
    if (Left == 0 || (Right > 0 && LeftAt->SortKey > RightAt->SortKey))
    {
     Less = RightAt;
     Assert(Right > 0);
     ++RightAt;
     --Right;
    }
    else
    {
     Less = LeftAt;
     Assert(Left > 0);
     ++LeftAt;
     --Left;
    }
    *NextAt++ = *Less;
   }
  }
  
  Swap(Curr, Next, sort_entry *);
 }
 
 return Curr;
}

internal int
SortEntryCmp(void *Data, sort_entry *A, sort_entry *B)
{
 int Result = Cmp(A->SortKey, B->SortKey);
 return Result;
}

internal void
Sort(sort_entry *Entries, u32 Count, sort_flags Flags)
{
 SortTyped(Entries, Count, SortEntryCmp, 0, Flags, sort_entry);
 
#if 0
 if (Flags & SortFlag_Stable)
 {
  temp_arena Temp = TempArena(0);
  sort_entry *TempMemory = PushArrayNonZero(Temp.Arena, Count, sort_entry);
  sort_entry *Sorted = MergeSortStable(Entries, Count, TempMemory);
  if (Sorted != Entries)
  {
   ArrayCopy(Entries, Sorted, Count);
  }
  EndTemp(Temp);
 }
 else
 {
  QuickSort(Entries, Count);
 }
#endif
}

internal void
QuickSort(void *Elems, u32 ElemSize, u32 Count, sort_cmp_func *CmpFunc, void *Data, void *SwapBuffer)
{
 if (Count > 0)
 { 
  u32 PivotIndex = Count - 1;
  void *PivotAt = AtIndexUntyped(Elems, PivotIndex, ElemSize);
  u32 Left = 0;
  void *ElemAt = Elems;
  
  for (u32 Index = 0;
       Index < Count;
       ++Index)
  {
   void *ElemAt = AtIndexUntyped(Elems, Index, ElemSize);
   if (CmpFunc(Data, ElemAt, PivotAt) <= 0)
   {
    void *ElemLeftAt = AtIndexUntyped(Elems, Left, ElemSize);
    SwapUntyped(ElemAt, ElemLeftAt, SwapBuffer, ElemSize);
    ++Left;
   }
  }
  
  Assert(Left > 0);
  QuickSort(AtIndexUntyped(Elems, 0, ElemSize), ElemSize, Left - 1, CmpFunc, Data, SwapBuffer);
  QuickSort(AtIndexUntyped(Elems, Left, ElemSize), ElemSize, Count - Left, CmpFunc, Data, SwapBuffer);
 }
}

internal void
__SortTyped(void *Array, u32 ElemSize, u32 Count,
            sort_cmp_func *CmpFunc, void *Data,
            sort_flags Flags, void *SwapBuffer)
{
 if (Flags & SortFlag_Stable)
 {
  NotImplemented;
 }
 else
 {
  QuickSort(Array, ElemSize, Count, CmpFunc, Data, SwapBuffer);
 }
}
