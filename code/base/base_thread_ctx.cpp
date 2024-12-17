struct thread_ctx
{
 arena **Arenas;
 u32 ArenaCount;
};
global thread_static thread_ctx GlobalThreadCtx;

internal void
ThreadCtxEquip(arena **Arenas, u32 ArenaCount)
{
 GlobalThreadCtx.Arenas = Arenas;
 GlobalThreadCtx.ArenaCount = ArenaCount;
}

internal temp_arena
ThreadCtxGetScratch(arena *Conflict)
{
 temp_arena Result = {};
 for (u32 Index = 0;
      Index < GlobalThreadCtx.ArenaCount;
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