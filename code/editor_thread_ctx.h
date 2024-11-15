#ifndef EDITOR_THREAD_CTX_H
#define EDITOR_THREAD_CTX_H

struct thread_ctx
{
 b32 Initialized;
 arena *Arenas[2];
};

internal void InitThreadCtx();
internal temp_arena TempArena(arena *Conflict);

#endif //EDITOR_THREAD_CTX_H
