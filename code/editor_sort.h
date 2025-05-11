#ifndef EDITOR_SORT_H
#define EDITOR_SORT_H

typedef f32 sort_key_f32;
struct sort_entry
{
 sort_key_f32 SortKey;
 u32 Index;
};

enum sort_order
{
 SortOrder_Ascending,
 SortOrder_Descending,
};

struct sort_entry_array
{
 sort_entry *Entries;
 u32 Count;
 u32 MaxCount;
 sort_order Order;
};

typedef int sort_cmp_func(void *Data, void *A, void *B);

enum
{
 SortFlag_None = 0,
 SortFlag_Stable = (1<<0),
};
typedef u32 sort_flags;

internal sort_entry_array AllocSortEntryArray(arena *Arena, u32 MaxCount, sort_order Order);
internal void             AddSortEntry(sort_entry_array *Array, sort_key_f32 SortKey, u32 Index);

internal void Sort(sort_entry *Entries, u32 Count, sort_flags Flags);
#define       SortTyped(Array, Count, CmpFunc, Data, Flags, type) do { type NameConcat(__SwapBuffer, __LINE__); MarkUnused(SizeOf(CmpFunc(Data, Cast(type *)0, Cast(type *)0))); /* this should compile */ __SortTyped(Array, SizeOf((Array)[0]), Count, Cast(sort_cmp_func *)CmpFunc, Data, Flags, &NameConcat(__SwapBuffer, __LINE__)); } while (0)

//- ignore
internal void __SortTyped(void *Array, u32 ElemSize, u32 Count, sort_cmp_func *CmpFunc, void *Data, sort_flags Flags, void *SwapBuffer);

#endif //EDITOR_SORT_H
