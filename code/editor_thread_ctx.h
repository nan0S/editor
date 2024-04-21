#ifndef EDITOR_THREAD_CTX_H
#define EDITOR_THREAD_CTX_H

typedef struct
{
   b32 Initialized;
   arena *Arenas[2];
} thread_ctx;

function void InitThreadCtx(u64 PerArenaCapacity);
function temp_arena TempArena(arena *Conflict);

#endif //EDITOR_THREAD_CTX_H
