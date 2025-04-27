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

inline internal int
SortEntryCmp(sort_entry *A, sort_entry *B)
{
 int Result = Cmp(A->SortKey, B->SortKey);
 return Result;
}

internal void
QuickSort(sort_entry *Entries, u64 Count)
{
 if (Count > 0)
 { 
  u64 PivotIndex = Count - 1;
  sort_key_f32 Pivot = Entries[PivotIndex].SortKey;
  
  u64 Left = 0;
  for (u64 Index = 0;
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
MergeSortStable(sort_entry *Entries, u64 Count, sort_entry *TempMemory)
{
 sort_entry *Curr = Entries;
 sort_entry *Next = TempMemory;
 
 for (u64 Length = 1;
      Length <= Count;
      Length <<= 1)
 {
  for (u64 Index = 0;
       Index < Count;
       Index += (Length << 1))
  {
   u64 LeftIndex = Index;
   u64 RightIndex = LeftIndex + Length;
   
   sort_entry *LeftAt = Curr + LeftIndex;
   sort_entry *RightAt = Curr + RightIndex;
   
   u64 Left = Min(Count - ClampTop(LeftIndex, Count), Length);
   u64 Right = Min(Count - ClampTop(RightIndex, Count), Length);
   u64 Total = Left + Right;
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

internal void
Sort(sort_entry_array Array)
{
 QuickSort(Array.Entries, Array.Count);
}

internal void
SortStable(sort_entry_array Array)
{
 temp_arena Temp = TempArena(0);
 
 sort_entry *TempMemory = PushArrayNonZero(Temp.Arena, Array.Count, sort_entry);
 sort_entry *Sorted = MergeSortStable(Array.Entries, Array.Count, TempMemory);
 if (Sorted != Array.Entries)
 {
  ArrayCopy(Array.Entries, Sorted, Array.Count);
 }
 
 EndTemp(Temp);
}
