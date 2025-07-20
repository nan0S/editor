internal string_store *
AllocStringStore(arena *PermamentArena)
{
 string_store *Store = PushStruct(PermamentArena, string_store);
 InitStringCache(&Store->StrCache);
 Store->Arena = PermamentArena;
 return Store;
}

internal string_id
AllocStringFromString(string_store *Store, string Str)
{
 string_id Id = AllocStringOfSize(Store, Str.Count);
 char_buffer *Buffer = CharBufferFromStringId(Store, Id);
 FillCharBuffer(Buffer, Str);
 return Id;
}

internal string_id
AllocStringOfSize(string_store *Store, u64 Size)
{
 char_buffer Buffer = AllocString(&Store->StrCache, Size);
 if (Store->StrCount >= Store->StrCapacity)
 {
  u32 NewCapacity = Max(2 * Store->StrCapacity, Store->StrCount + 1);
  char_buffer *NewStrs = PushArrayNonZero(Store->Arena, NewCapacity, char_buffer);
  ArrayCopy(NewStrs, Store->Strs, Store->StrCount);
  Store->Strs = NewStrs;
  Store->StrCapacity = NewCapacity;
 }
 Assert(Store->StrCount < Store->StrCapacity);
 Store->Strs[Store->StrCount] = Buffer;
 u32 Index = Store->StrCount++;
 string_id Id = {Index};
 return Id;
}

internal char_buffer *
CharBufferFromStringId(string_store *Store, string_id Id)
{
 Assert(Id.Index < Store->StrCount);
 char_buffer *Buffer = Store->Strs + Id.Index;
 return Buffer;
}

internal string
StringFromStringId(string_store *Store, string_id Id)
{
 char_buffer *Buffer = CharBufferFromStringId(Store, Id);
 string Str = StrFromCharBuffer(*Buffer);
 return Str;
}
