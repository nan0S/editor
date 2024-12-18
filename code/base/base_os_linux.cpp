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

internal os_file_handle OS_FileOpen(string Path, file_access_flags Access) { NotImplemented; return {}; }
internal void           OS_FileClose(os_file_handle File) { NotImplemented; }
internal u64            OS_FileRead(os_file_handle File, char *Buf, u64 Read, u64 Offset) { NotImplemented; return {}; }
internal u64            OS_FileWrite(os_file_handle File, char *Buf, u64 Write, u64 Offset) { NotImplemented; return {}; }
internal u64            OS_FileSize(os_file_handle File) { NotImplemented; return {}; }
internal file_attrs     OS_FileAttributes(string Path) { NotImplemented; return {}; }

internal os_file_handle OS_StdOut(void) { NotImplemented; return {}; }
internal os_file_handle OS_StdError(void) { NotImplemented; return {}; }

internal void OS_PrintDebug(string Str) { NotImplemented; }

internal void   OS_FileDelete(string Path) { NotImplemented; }
internal b32    OS_FileMove(string Src, string Dest) { NotImplemented; return {}; }
internal b32    OS_FileCopy(string Src, string Dest) { NotImplemented; return {}; }
internal b32    OS_FileExists(string Path) { NotImplemented; return {}; }
internal b32    OS_DirMake(string Path) { NotImplemented; return {}; }
internal b32    OS_DirRemove(string Path) { NotImplemented; return {}; }
internal b32    OS_DirChange(string Path) { NotImplemented; return {}; }
internal string OS_CurrentDir(arena *Arena) { NotImplemented; return {}; }
internal string OS_FullPathFromPath(arena *Arena, string Path) { NotImplemented; return {}; }

internal os_library_handle OS_LibraryLoad(string Path) { NotImplemented; return {}; }
internal void *            OS_LibraryProc(os_library_handle Lib, char const *ProcName) { NotImplemented; return {}; }
internal void              OS_LibraryUnload(os_library_handle Lib) { NotImplemented; }

internal os_process_handle OS_ProcessLaunch(string_list CmdList) { NotImplemented; return {}; }
internal b32               OS_ProcessWait(os_process_handle Process) { NotImplemented; return {}; }
internal void              OS_ProcessCleanup(os_process_handle Handle) { NotImplemented; }

internal os_thread_handle OS_ThreadLaunch(os_thread_func *Func, void *Data) { NotImplemented; return {}; }
internal void             OS_ThreadWait(os_thread_handle Thread) { NotImplemented; }
internal void             OS_ThreadRelease(os_thread_handle Thread) { NotImplemented; }

internal void OS_MutexAlloc(os_mutex_handle *Mutex) { NotImplemented; }
internal void OS_MutexLock(os_mutex_handle *Mutex) { NotImplemented; }
internal void OS_MutexUnlock(os_mutex_handle *Mutex) { NotImplemented; }
internal void OS_MutexDealloc(os_mutex_handle *Mutex) { NotImplemented; }

internal void OS_SemaphoreAlloc(os_semaphore_handle *Sem, u32 InitialCount, u32 MaxCount) { NotImplemented; }
internal void OS_SemaphorePost(os_semaphore_handle *Sem) { NotImplemented; }
internal void OS_SemaphoreWait(os_semaphore_handle *Sem) { NotImplemented; }
internal void OS_SemaphoreDealloc(os_semaphore_handle *Sem) { NotImplemented; }

internal void OS_BarrierAlloc(os_barrier_handle *Barrier, u32 ThreadCount) { NotImplemented; }
internal void OS_BarrierWait(os_barrier_handle *Barrier) { NotImplemented; }
internal void OS_BarrierDealloc(os_barrier_handle *Barrier) { NotImplemented; }

internal u64 OS_AtomicIncr64(u64 volatile *Value) { NotImplemented; return {}; }
internal u64 OS_AtomicAdd64(u64 volatile *Value, u64 Add) { NotImplemented; return {}; }
internal u64 OS_AtomicCmpExch64(u64 volatile *Value, u64 Cmp, u64 Exch) { NotImplemented; return {}; }

internal u32 OS_AtomicIncr32(u32 volatile *Value) { NotImplemented; return {}; }
internal u32 OS_AtomicAdd32(u32 volatile *Value, u32 Add) { NotImplemented; return {}; }
internal u32 OS_AtomicCmpExch32(u32 volatile *Value, u32 Cmp, u32 Exch) { NotImplemented; return {}; }

internal u64 OS_ReadCPUTimer(void) { NotImplemented; return {}; }
internal u64 OS_CPUTimerFreq(void) { NotImplemented; return {}; }
internal u64 OS_ReadOSTimer(void) { NotImplemented; return {}; }
internal u64 OS_OSTimerFreq(void) { NotImplemented; return {}; }

internal u32 OS_ProcCount(void) { NotImplemented; return {}; }
internal u32 OS_PageSize(void) { NotImplemented; return {}; }
internal u32 OS_ThreadGetID(void) { NotImplemented; return {}; }

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