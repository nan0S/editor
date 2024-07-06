#include "editor_base.h"
#include "editor_os.h"
#include "editor_memory.h"

#include "editor_base.cpp"
#include "editor_os.cpp"

int main()
{
   arena *Arena = AllocArena();
   void *Memory = PushSize(Arena, 10);
   char *Buffer = Cast(char *)Memory;
   for (u64 I = 0; I < 10; ++I)
   {
      Buffer[I] = I;
   }
   
   return 0;
}