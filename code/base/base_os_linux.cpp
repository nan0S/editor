internal void *
OS_Reserve(u64 Reserve)
{
 void *Result = mmap(0, Reserve, PROT_NONE, MAP_ANON|MAP_PRIVATE, -1, 0);
 return Result;
}

internal void
OS_Release(void *Memory, u64 Size)
{
 munmap(Memory, Size);
}

internal void
OS_Commit(void *Memory, u64 Size)
{
 mprotect(Memory, Size, PROT_READ|PROT_WRITE);
}

internal b32
OS_IterDir(string Path, dir_iter *Iter, dir_entry *OutEntry)
{
 b32 Found = false;
 
 if (!Iter->NotFirstTime)
 {
  temp_arena Temp = TempArena(0);
  string CPath = CStrFromStr(Temp.Arena, Path);
  Iter->Dir = opendir(CPath.Data);
  EndTemp(Temp);
 }
 
 if (Iter->Dir)
 {
  b32 Looking = true;
  while (Looking)
  {
   struct dirent *Entry = readdir(Iter->Dir);
   if (Entry)
   {
    OutEntry->FileName = StrFromCStr(Entry->d_name);
    // TODO(hbr): Use d_reclen to extaract other information about the file
    OutEntry->Attrs.Dir = (Entry->d_type == DT_DIR);
    Found = true;
    Looking = false;
   }
   else
   {
    Looking = false;
   }
  }
 }
 
 if (!Found)
 {
  closedir(Iter->Dir);
 }
 
 return Found;
}

#if 0
internal os_file_handle
OS_FileClose(string Path, file_access_flags Access)
{
 NotImplemented;
 return {};
}
#endif

#if 0
// SPDX-FileCopyrightText: © 2022 Phillip Trudeau-Tavara <pmttavara@protonmail.com>
// SPDX-License-Identifier: 0BSD

// https://linux.die.net/man/2/perf_event_open
// https://stackoverflow.com/a/57835630

#include <stdbool.h>
#include <stdint.h>

#include <sys/mman.h>
#include <linux/perf_event.h>
#include <time.h>
#include <unistd.h>
#include <x86intrin.h>

static inline uint64_t get_rdtsc_freq(void) {
 
 // Cache the answer so that multiple calls never take the slow path more than once
 static uint64_t tsc_freq = 0;
 if (tsc_freq) {
  return tsc_freq;
 }
 
 // Fast path: Load kernel-mapped memory page
 struct perf_event_attr pe = {0};
 pe.type = PERF_TYPE_HARDWARE;
 pe.size = sizeof(struct perf_event_attr);
 pe.config = PERF_COUNT_HW_INSTRUCTIONS;
 pe.disabled = 1;
 pe.exclude_kernel = 1;
 pe.exclude_hv = 1;
 
 // __NR_perf_event_open == 298 (on x86_64)
 int fd = syscall(298, &pe, 0, -1, -1, 0);
 if (fd != -1) {
  
  struct perf_event_mmap_page *pc = (struct perf_event_mmap_page *)mmap(NULL, 4096, PROT_READ, MAP_SHARED, fd, 0);
  if (pc) {
   
   // success
   if (pc->cap_user_time == 1) {
    // docs say nanoseconds = (tsc * time_mult) >> time_shift
    //      set nanoseconds = 1000000000 = 1 second in nanoseconds, solve for tsc
    //       =>         tsc = 1000000000 / (time_mult >> time_shift)
    tsc_freq = (1000000000ull << (pc->time_shift / 2)) / (pc->time_mult >> (pc->time_shift - pc->time_shift / 2));
    // If your build configuration supports 128 bit arithmetic, do this:
    // tsc_freq = ((__uint128_t)1000000000ull << (__uint128_t)pc->time_shift) / pc->time_mult;
   }
   munmap(pc, 4096);
  }
  close(fd);
 }
 
 // Slow path
 if (!tsc_freq) {
  
  // Get time before sleep
  uint64_t nsc_begin = 0; { struct timespec t; if (!clock_gettime(CLOCK_MONOTONIC_RAW, &t)) nsc_begin = (uint64_t)t.tv_sec * 1000000000ull + t.tv_nsec; }
  uint64_t tsc_begin = __rdtsc();
  
  usleep(10000); // 10ms gives ~4.5 digits of precision - the longer you sleep, the more precise you get
  
  // Get time after sleep
  uint64_t nsc_end = nsc_begin + 1; { struct timespec t; if (!clock_gettime(CLOCK_MONOTONIC_RAW, &t)) nsc_end = (uint64_t)t.tv_sec * 1000000000ull + t.tv_nsec; }
  uint64_t tsc_end = __rdtsc();
  
  // Do the math to extrapolate the RDTSC ticks elapsed in 1 second
  tsc_freq = (tsc_end - tsc_begin) * 1000000000 / (nsc_end - nsc_begin);
 }
 
 // Failure case
 if (!tsc_freq) {
  tsc_freq = 1000000000;
 }
 
 return tsc_freq;
}

#include <stdio.h>
int main() {
 printf("Calling get_rdtsc_freq() 1,000 times...\n");
 uint64_t start = __rdtsc();
 uint64_t freq = 0;
 for (int i = 0; i < 1000; i++) {
  freq = get_rdtsc_freq();
 }
 uint64_t end = __rdtsc();
 printf("...took %f milliseconds.\n", (end - start) * 1000.0 / freq);
 printf("RDTSC frequency is %lu (%.2f GHz).\n", (unsigned long)freq, freq * 0.000000001);
}
#endif