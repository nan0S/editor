#ifndef EDITOR_LINUX_H
#define EDITOR_LINUX_H

#include <sys/mman.h>
#include <x86intrin.h>
#include <sys/time.h>

struct dir_iter
{
   b32 NotFirstTime;
   DIR *Dir;
};

#endif //EDITOR_LINUX_H
