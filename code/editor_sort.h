#ifndef EDITOR_SORT_H
#define EDITOR_SORT_H

typedef f32 sort_key;
struct sort_entry
{
 sort_key SortKey;
 u64 Index;
};

struct sorted_entries
{
 u64 Count;
 sort_entry *Entries;
};

inline internal int
SortEntryCmp(sort_entry *A, sort_entry *B)
{
 int Result = Cmp(A->SortKey, B->SortKey);
 return Result;
}

internal void
X_QuickSort(sort_entry *Entries, u64 Count)
{
 if (Count > 0)
 { 
  u64 PivotIndex = Count - 1;
  sort_key Pivot = Entries[PivotIndex].SortKey;
  
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
  X_QuickSort(Entries, Left - 1);
  X_QuickSort(Entries + Left, Count - Left);
 }
}

internal void
MergeSortStable(sort_entry *Entries, u64 Count, sort_entry *TempMemory)
{
 if (Count > 1)
 {
  u64 Left = Count / 2;
  u64 Right = Count - Left;
  
  sort_entry *LeftAt = Entries;
  sort_entry *RightAt = Entries + Left;
  
  MergeSortStable(LeftAt, Left, TempMemory);
  MergeSortStable(RightAt, Right, TempMemory);
  
  for (u64 Index = 0;
       Index < Count;
       ++Index)
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
   TempMemory[Index] = *Less;
  }
  
  ArrayCopy(Entries, TempMemory, Count);
 }
}

#endif //EDITOR_SORT_H
