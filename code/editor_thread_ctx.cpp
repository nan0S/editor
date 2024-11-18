struct thread_ctx
{
 arena *Arenas[2];
};
global thread_static thread_ctx GlobalThreadCtx;

internal void
ThreadCtxAlloc(void)
{
 for (u32 ArenaIndex = 0;
      ArenaIndex < ArrayCount(GlobalThreadCtx.Arenas);
      ++ArenaIndex)
 {
  GlobalThreadCtx.Arenas[ArenaIndex] = AllocArena();
 }
}

internal void
ThreadCtxDealloc(void)
{
 for (u32 ArenaIndex = 0;
      ArenaIndex < ArrayCount(GlobalThreadCtx.Arenas);
      ++ArenaIndex)
 {
  DeallocArena(GlobalThreadCtx.Arenas[ArenaIndex]);
 }
}

internal temp_arena
ThreadCtxTempArena(arena *Conflict)
{
 temp_arena Result = {};
 for (u32 Index = 0;
      Index < ArrayCount(GlobalThreadCtx.Arenas);
      ++Index)
 {
  arena *Arena = GlobalThreadCtx.Arenas[Index];
  if (Arena != Conflict)
  {
   Result = BeginTemp(Arena);
   break;
  }
 }
 
 return Result;
}