#ifndef BASE_THREAD_CTX_H
#define BASE_THREAD_CTX_H

internal void ThreadCtxEquid(arena **Arenas, u32 ArenaCount);
internal temp_arena ThreadCtxGetScratch(arena *Conflict);

#ifndef TempArena
# define TempArena ThreadCtxGetScratch
#endif

#endif //BASE_THREAD_CTX_H
