global thread_local thread_ctx GlobalCtx;

function void
InitThreadCtx(u64 PerArenaCapacity)
{
   if (!GlobalCtx.Initialized)
   {
      for (u64 ArenaIndex = 0;
           ArenaIndex < ArrayCount(GlobalCtx.Arenas);
           ++ArenaIndex)
      {
         GlobalCtx.Arenas[ArenaIndex] = AllocateArena(PerArenaCapacity);
      }
      
      GlobalCtx.Initialized = true;
   }
}

function temp_arena
TempArena(arena *Conflict)
{
   temp_arena Result = {};
   for (u64 Index = 0;
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