#ifndef BASE_THREAD_CTX_H
#define BASE_THREAD_CTX_H

struct thread_ctx
{
 arena *Arenas[2];
};

internal void ThreadCtxInit(void);
internal temp_arena ThreadCtxGetScratch(arena *Conflict);

#endif //BASE_THREAD_CTX_H
