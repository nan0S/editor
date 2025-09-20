/* ========================================================================
   Parametric Curve Editor
   Master's Thesis by Hubert Obrzut
   Supervisor: Paweł Woźny
   University of Wrocław
   Faculty of Mathematics and Computer Science
   Institute of Computer Science
   Date: September 2025
   ======================================================================== */

global thread_static thread_ctx GlobalThreadCtxThreadLocal;

internal thread_ctx *
ThreadCtxGet(void)
{
 return &GlobalThreadCtxThreadLocal;
}

internal void
ThreadCtxInit(void)
{
 thread_ctx *Ctx = ThreadCtxGet();
 for (u32 ArenaIndex = 0;
      ArenaIndex < ArrayCount(Ctx->Arenas);
      ++ArenaIndex)
 {
  Ctx->Arenas[ArenaIndex] = AllocArena(Gigabytes(64));
 }
}

internal temp_arena
ThreadCtxGetScratch(arena *Conflict)
{
 thread_ctx *Ctx = ThreadCtxGet();
 temp_arena Result = {};
 
 for (u32 Index = 0;
      Index < ArrayCount(Ctx->Arenas);
      ++Index)
 {
  arena *Arena = Ctx->Arenas[Index];
  if (Arena != Conflict)
  {
   Result = BeginTemp(Arena);
   break;
  }
 }
 
 return Result;
}

#ifndef TempArena
# define TempArena ThreadCtxGetScratch
#endif