global thread_static thread_ctx GlobalCtx;

internal void
InitThreadCtx()
{
 if (!GlobalCtx.Initialized)
 {
  for (u32 ArenaIndex = 0;
       ArenaIndex < ArrayCount(GlobalCtx.Arenas);
       ++ArenaIndex)
  {
   GlobalCtx.Arenas[ArenaIndex] = AllocArena();
  }
  GlobalCtx.Initialized = true;
 }
}

internal temp_arena
TempArena(arena *Conflict)
{
 temp_arena Result = {};
 for (u32 Index = 0;
      Index < ArrayCount(GlobalCtx.Arenas);
      ++Index)
 {
  arena *Arena = GlobalCtx.Arenas[Index];
  if (Arena != Conflict)
  {
   Result = BeginTemp(Arena);
   break;
  }
 }
 
 return Result;
}