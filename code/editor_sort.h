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

internal sort_entry_array AllocSortEntryArray(arena *Arena, u32 MaxCount, sort_order Order);
internal void AddSortEntry(sort_entry_array *Array, sort_key_f32 SortKey, u32 Index);
internal void Sort(sort_entry_array Array);
internal void SortStable(sort_entry_array Array);

#endif //EDITOR_SORT_H
