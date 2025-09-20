/* ========================================================================
   Parametric Curve Editor
   Master's Thesis by Hubert Obrzut
   Supervisor: Paweł Woźny
   University of Wrocław
   Faculty of Mathematics and Computer Science
   Institute of Computer Science
   Date: September 2025
   ======================================================================== */

#ifndef BASE_THREAD_CTX_H
#define BASE_THREAD_CTX_H

struct thread_ctx
{
 arena *Arenas[2];
};

internal void ThreadCtxInit(void);
internal temp_arena ThreadCtxGetScratch(arena *Conflict);

#endif //BASE_THREAD_CTX_H
