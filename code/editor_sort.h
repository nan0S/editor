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

// TODO(hbr): improve, this was implemented quickly
internal void
MergeSortStable(sort_entry *Cur, u64 Count, sort_entry *Next)
{
 sort_entry *Entries = Cur;
 sort_entry *TempMemory = Next;
 
 for (u64 Length = 1;
      Length <= Count;
      Length <<= 1)
 {
  for (u64 Index = 0;
       Index < Count;
       Index += (Length << 1))
  {
   sort_entry *LeftAt = Entries + Index;
   sort_entry *RightAt = Entries + Index + Length;
   
   u64 Left = Length;
   u64 Right = Min(Count - Index, Length);
   u64 Total = Left + Right;
   sort_entry *TempMemoryAt = TempMemory + Index;
   
   for (u64 Merge = 0;
        Merge < Total;
        ++Merge)
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
    *TempMemoryAt++ = *Less;
   }
  }
  
  Swap(Entries, TempMemory, sort_entry *);
 }
 
 // NOTE(hbr): Entries are sorted here, do one more ArrayCopy if it isn't Cur
 if (Entries != Cur)
 {
  ArrayCopy(Cur, Entries, Count);
 }
}

#endif //EDITOR_SORT_H
