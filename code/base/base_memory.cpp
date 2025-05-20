internal void
InitStringCache(string_cache *Cache)
{
 Cache->Arena = AllocArena(Gigabytes(64));
}

internal u64
FindBigEnoughStringChunkBucket(u64 Size)
{
 u64 FoundIndex = 0;
 ForEachElement(Index, StringBucketChunkSizes)
 {
  if (StringBucketChunkSizes[Index] >= Size)
  {
   FoundIndex = Index;
   break;
  }
 }
 return FoundIndex;
}

internal char_buffer
AllocString(string_cache *Cache, u64 Size)
{
 char_buffer Result = {};
 
 u64 BucketIndex = FindBigEnoughStringChunkBucket(Size);
 u64 AllocSize = 0;
 string_chunk_node *AllocNode = 0;
 string_chunk_node *AllocPrevNode = 0;
 
 if (BucketIndex == ArrayCount(StringBucketChunkSizes) - 1)
 {
  string_chunk_node *BestNode = Cache->FreeStringChunks[BucketIndex];
  string_chunk_node *BestPrevNode = 0;
  string_chunk_node *PrevNode = 0;
  ListIter(Node, Cache->FreeStringChunks[BucketIndex], string_chunk_node)
  {
   if (Node->Size >= Size && Node->Size < BestNode->Size)
   {
    BestNode = Node;
    BestPrevNode = PrevNode;
   }
   PrevNode = Node;
  }
  AllocNode = BestNode;
  AllocPrevNode = BestPrevNode;
  AllocSize = Size;
 }
 else
 {
  AllocNode = Cache->FreeStringChunks[BucketIndex];
  AllocSize = StringBucketChunkSizes[BucketIndex];
 }
 
 char *Data = 0;
 if (AllocNode)
 {
  Data = Cast(char *)AllocNode;
  if (AllocPrevNode)
  {
   AllocPrevNode->Next = AllocNode->Next;
  }
  else
  {
   Cache->FreeStringChunks[BucketIndex] = AllocNode->Next;
  }
 }
 else
 {
  Data = PushArrayNonZero(Cache->Arena, AllocSize, char);
 }
 
 Result.Data = Data;
 Result.Count = Size;
 Result.Capacity = AllocSize;
 
 return Result;
}

internal void
DeallocString(string_cache *Cache, char_buffer Buffer)
{
 if (Buffer.Count)
 {
  string_chunk_node *Node = Cast(string_chunk_node *)Buffer.Data;
  Node->Size = Buffer.Capacity;
  u64 BucketIndex = FindBigEnoughStringChunkBucket(Buffer.Capacity);
  StackPush(Cache->FreeStringChunks[BucketIndex], Node);
 }
}