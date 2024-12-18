#include "base/base_ctx_crack.h"
#include "base/base_core.h"
#include "base/base_string.h"
#include "base/base_arena.h"
#include "base/base_thread_ctx.h"
#include "base/base_os.h"

#include "base/base_string.cpp"
#include "base/base_arena.cpp"
#include "base/base_thread_ctx.cpp"
#include "base/base_os.cpp"

int main()
{
 arena *Arenas[2] = {};
 for (u32 ArenaIndex = 0; ArenaIndex < ArrayCount(Arenas); ++ArenaIndex)
 {
  Arenas[ArenaIndex] = AllocArena(Gigabytes(64));
 } 
 ThreadCtxEquip(Arenas, ArrayCount(Arenas));

 arena *Arena = Arenas[0];
 string A = OS_FullPathFromPath(Arena, StrLit("."));
 string B = OS_FullPathFromPath(Arena, StrLit(".."));
 string C = OS_FullPathFromPath(Arena, StrLit("../code/build.cpp"));

 return 0;
}