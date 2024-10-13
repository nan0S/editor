#ifndef EDITOR_SORT_H
#define EDITOR_SORT_H

struct sort_entry
{
   s64 SortKey;
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
   int Result = IntCmp(A->Index, B->Index);
   return Result;
}

#endif //EDITOR_SORT_H
