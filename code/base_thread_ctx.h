#ifndef BASE_THREAD_CTX_H
#define BASE_THREAD_CTX_H

struct thread_ctx
{
 b32 Initialized;
 arena *Arenas[2];
};

internal void InitThreadCtx(u64 ReservePerArena);
internal temp_arena TempArena(arena *Conflict);

#endif //BASE_THREAD_CTX_H
