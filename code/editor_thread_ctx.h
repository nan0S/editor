#ifndef EDITOR_THREAD_CTX_H
#define EDITOR_THREAD_CTX_H

internal void ThreadCtxAlloc(void);
internal void ThreadCtxDealloc(void);
internal temp_arena ThreadCtxTempArena(arena *Conflict);

#endif //EDITOR_THREAD_CTX_H
