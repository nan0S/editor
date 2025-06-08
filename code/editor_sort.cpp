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

internal void *
MergeSortStable(void *Elems, u32 ElemSize, u32 Count, void *Temp, sort_cmp_func *CmpFunc, void *Data)
{
 void *Curr = Elems;
 void *Next = Temp;
 
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
   
   void *LeftAt = AtIndexUntyped(Curr, LeftIndex, ElemSize);
   void *RightAt = AtIndexUntyped(Curr, RightIndex, ElemSize);
   
   u32 Left = Min(Count - ClampTop(LeftIndex, Count), Length);
   u32 Right = Min(Count - ClampTop(RightIndex, Count), Length);
   u32 Total = Left + Right;
   void *NextAt = AtIndexUntyped(Next, Index, ElemSize);
   
   while (Total--)
   {
    void *Less;
    if (Left == 0 || (Right > 0 &&  CmpFunc(Data, LeftAt, RightAt) > 0))
    {
     Less = RightAt;
     Assert(Right > 0);
     RightAt = AdvancedPtrUntyped(RightAt, ElemSize);
     --Right;
    }
    else
    {
     Less = LeftAt;
     Assert(Left > 0);
     LeftAt = AdvancedPtrUntyped(LeftAt, ElemSize);
     --Left;
    }
    
    MemoryCopy(NextAt, Less, ElemSize);
    NextAt = AdvancedPtrUntyped(NextAt, ElemSize);
   }
  }
  
  Swap(Curr, Next, void *);
 }
 
 return Curr;
}

internal int
SortEntryCmp(void *Data, sort_entry *A, sort_entry *B)
{
 MarkUnused(Data);
 int Result = Cmp(A->SortKey, B->SortKey);
 return Result;
}

internal void
Sort(sort_entry *Entries, u32 Count, sort_flags Flags)
{
 SortTyped(Entries, Count, SortEntryCmp, 0, Flags, sort_entry);
}

internal void
QuickSort(void *Elems, u32 ElemSize, u32 Count, sort_cmp_func *CmpFunc, void *Data, void *SwapBuffer)
{
 if (Count > 0)
 { 
  u32 PivotIndex = Count - 1;
  void *PivotAt = AtIndexUntyped(Elems, PivotIndex, ElemSize);
  u32 Left = 0;
  
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
  temp_arena Temp = TempArena(0);
  void *TempMemory = PushArrayUntypedNonZero(Temp.Arena, Count, ElemSize);
  void *Sorted = MergeSortStable(Array, ElemSize, Count, TempMemory, CmpFunc, Data);
  if (Sorted != Array)
  {
   ArrayCopyUntyped(Array, Sorted, Count, ElemSize);
  }
  EndTemp(Temp);
 }
 else
 {
  QuickSort(Array, ElemSize, Count, CmpFunc, Data, SwapBuffer);
 }
}
