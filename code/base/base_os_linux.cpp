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

internal os_file_handle
OS_FileOpen(string Path, file_access_flags Access)
{
 temp_arena Temp = TempArena(0);
 string CPath = CStrFromStr(Temp.Arena, Path);
 int Flags = 0;
 if (Access & FileAccess_Read && Access & FileAccess_Write) Flags = O_RDWR;
 else if (Access & FileAccess_Read) Flags = O_RDONLY;
 else if (Access & FileAccess_Write) Flags = O_WRONLY;
 int File = open(CPath.Data, Flags);
 EndTemp(Temp);
 return File;
}

internal void
OS_FileClose(os_file_handle File)
{
 close(File);
}

typedef ssize_t linux_file_op_func(int fd, void *buf, size_t count);

internal u64
OS_FileOperation(linux_file_op_func *Op, os_file_handle File, char *Buf, u64 Target, u64 Offset)
{
 u64 Processed = 0;

 if (Offset != U64_MAX)
 {
  lseek(File, Offset, SEEK_SET);
 }

 u64 Left = Target;
 char *At = Buf;
 u64 OffsetAt = Offset;
 while (Left)
 {
  u32 ToProcess = Left;
  ssize_t ActuallyProcessed = Op(File, At, ToProcess);

  if (ActuallyProcessed > 0)
  { 
    Left -= ActuallyProcessed;
    At += ActuallyProcessed;
    OffsetAt += ActuallyProcessed;
    Processed += ActuallyProcessed;
  }
  else
  {
    break;
  }
  
  if (ActuallyProcessed != ToProcess)
  {
   break;
  }
 }
 
 return Processed;
}

internal u64
OS_FileRead(os_file_handle File, char *Buf, u64 Read, u64 Offset)
{
 u64 ReadCount = OS_FileOperation(read, File, Buf, Read, Offset);
 return ReadCount;
}

internal u64
OS_FileWrite(os_file_handle File, char *Buf, u64 Write, u64 Offset)
{
  u64 Written = OS_FileOperation(Cast(linux_file_op_func *)write, File, Buf, Write, Offset);
  return Written;
}

internal u64
OS_FileSize(os_file_handle File)
{
 struct stat Stat = {};
 fstat(File, &Stat);
 u64 Result = Stat.st_size;
 return Result;
}

internal timestamp64
LinuxFileTimeToTimestamp(time_t FileTime)
{
 timestamp64 Result = 0;
 struct tm *Time = localtime(&FileTime);
 if (Time)
 {
  date_time Date = {};
  Date.Sec = SafeCastU8(Time->tm_sec);
  Date.Mins = SafeCastU8(Time->tm_min);
  Date.Hour = SafeCastU8(Time->tm_hour);
  Date.Day = SafeCastU8(Time->tm_mday - 1);
  Date.Month = SafeCastU8(Time->tm_mon);
  Date.Year = 1900 + Time->tm_year;

  Result = DateTimeToTimestamp(Date);
 }

 return Result;
}

internal file_attrs
OS_FileAttributes(string Path)
{
 file_attrs Result = {};
 temp_arena Temp = TempArena(0);
 string CPath = CStrFromStr(Temp.Arena, Path);

 struct stat Stat = {};
 stat(CPath.Data, &Stat);

 Result.CreateTime = 0; // NOTE(hbr): creation time on Linux is not available
 Result.ModifyTime = LinuxFileTimeToTimestamp(Stat.st_mtime);
 Result.FileSize = Stat.st_size;
 Result.Dir = false;

 EndTemp(Temp);

 return Result;
}

internal os_file_handle OS_StdOut(void) { return STDOUT_FILENO; }
internal os_file_handle OS_StdError(void) { return STDERR_FILENO; }

internal void OS_PrintDebug(string Str) { OS_PrintErrorF("[DEBUG]: %S", Str); }

internal void   OS_FileDelete(string Path) { NotImplemented; }
internal b32    OS_FileMove(string Src, string Dest) { NotImplemented; return {}; }
internal b32    OS_FileCopy(string Src, string Dest) { NotImplemented; return {}; }

internal b32
OS_FileExists(string Path)
{
 temp_arena Temp = TempArena(0);
 string CPath = CStrFromStr(Temp.Arena, Path);
 int Ret = access(CPath.Data, R_OK);
 b32 Exists = (Ret == 0);
 EndTemp(Temp);
 return Exists;
}

internal b32    OS_DirMake(string Path) { NotImplemented; return {}; }
internal b32    OS_DirRemove(string Path) { NotImplemented; return {}; }
internal b32    OS_DirChange(string Path) { NotImplemented; return {}; }
internal string OS_CurrentDir(arena *Arena) { NotImplemented; return {}; }

internal string
OS_FullPathFromPath(arena *Arena, string Path)
{
 string Result = {};
 temp_arena Temp = TempArena(Arena);
 string CPath = CStrFromStr(Temp.Arena, Path);
 char *Buffer = PushArray(Arena, PATH_MAX+1, char);
 char *Resolved = realpath(CPath.Data, Buffer);
 if (Resolved)
 {
  Result = StrFromCStr(Resolved);
 }
 EndTemp(Temp);
 return Result;
}

internal os_library_handle OS_LibraryLoad(string Path) { NotImplemented; return {}; }
internal void *            OS_LibraryProc(os_library_handle Lib, char const *ProcName) { NotImplemented; return {}; }
internal void              OS_LibraryUnload(os_library_handle Lib) { NotImplemented; }

internal os_process_handle
OS_ProcessLaunch(string_list CmdList)
{
 temp_arena Temp = TempArena(0);
 string Cmd = StrListJoin(Temp.Arena, &CmdList, StrLit(" "));
 // TODO(hbr): use execve instead
 system(Cmd.Data);
 EndTemp(Temp);
 return 0;
}

internal b32
OS_ProcessWait(os_process_handle Process)
{
 // TODO(hbr): implement this
 return true;
}

internal void
OS_ProcessCleanup(os_process_handle Handle)
{
 // TODO(hbr): implement this
}

internal os_thread_handle
OS_ThreadLaunch(os_thread_func *Func, void *Data)
{
 pthread_t Thread = {};
 pthread_create(&Thread, 0, Func, Data);
 return Thread;
}

internal void
OS_ThreadWait(os_thread_handle Thread)
{
 NotImplemented;
}

internal void
OS_ThreadRelease(os_thread_handle Thread)
{
 NotImplemented;
}

internal void OS_MutexAlloc(os_mutex_handle *Mutex) { NotImplemented; }
internal void OS_MutexLock(os_mutex_handle *Mutex) { NotImplemented; }
internal void OS_MutexUnlock(os_mutex_handle *Mutex) { NotImplemented; }
internal void OS_MutexDealloc(os_mutex_handle *Mutex) { NotImplemented; }

internal void
OS_SemaphoreAlloc(os_semaphore_handle *Sem, u32 InitialCount, u32 MaxCount)
{
 sem_init(Sem, 0, InitialCount);
}

internal void
OS_SemaphorePost(os_semaphore_handle *Sem)
{
 sem_post(Sem);
}

internal void
OS_SemaphoreWait(os_semaphore_handle *Sem)
{
 sem_wait(Sem);
}

internal void
OS_SemaphoreDealloc(os_semaphore_handle *Sem)
{
 sem_destroy(Sem);
}

internal void OS_BarrierAlloc(os_barrier_handle *Barrier, u32 ThreadCount) { NotImplemented; }
internal void OS_BarrierWait(os_barrier_handle *Barrier) { NotImplemented; }
internal void OS_BarrierDealloc(os_barrier_handle *Barrier) { NotImplemented; }

internal inline u64
OS_AtomicIncr64(u64 volatile *Value)
{
 u64 Result = OS_AtomicAdd64(Value, 1); // NOTE(hbr): I don't think Linux has builtin increment function
 return Result;
}

#define LinuxAtomicAdd(Value, Add) __sync_fetch_and_add(Value, Add) + Add
#define LinuxAtomicCmpExch(Value, Cmp, Exch) __sync_val_compare_and_swap(Value, Cmp, Exch)

internal inline u64
OS_AtomicAdd64(u64 volatile *Value, u64 Add)
{
 u64 Result = __sync_fetch_and_add(Value, Add) + Add;
 return Result;
}

internal inline u64
OS_AtomicCmpExch64(u64 volatile *Value, u64 Cmp, u64 Exch)
{
 u64 Result = __sync_val_compare_and_swap(Value, Cmp, Exch);
 return Result;
}

internal u32
OS_AtomicIncr32(u32 volatile *Value)
{
 u64 Result = OS_AtomicAdd32(Value, 1); // NOTE(hbr): I don't think Linux has builtin increment function
 return Result;
}

internal inline u32
OS_AtomicAdd32(u32 volatile *Value, u32 Add)
{
 u32 Result = __sync_fetch_and_add(Value, Add) + Add;
 return Result;
}

internal inline u32
OS_AtomicCmpExch32(u32 volatile *Value, u32 Cmp, u32 Exch)
{
 u32 Result = __sync_val_compare_and_swap(Value, Cmp, Exch);
 return Result;
}

internal inline u64
OS_ReadCPUTimer(void)
{
 u64 Result = __rdtsc();
 return Result;
}

internal inline u64
OS_CPUTimerFreq(void)
{
 local u64 TSCFreq = 0;
 if (!TSCFreq)
 {
  //- Fast path: Load kernel-mapped memory page
  struct perf_event_attr PE = {0};
  PE.type = PERF_TYPE_HARDWARE;
  PE.size = sizeof(struct perf_event_attr);
  PE.config = PERF_COUNT_HW_INSTRUCTIONS;
  PE.disabled = 1;
  PE.exclude_kernel = 1;
  PE.exclude_hv = 1;
  
  // __NR_perf_event_open == 298 (on x86_64)
  int fd = syscall(298, &PE, 0, -1, -1, 0);
  if (fd != -1)
  { 
   struct perf_event_mmap_page *Page = Cast(struct perf_event_mmap_page *)mmap(NULL, 4096, PROT_READ, MAP_SHARED, fd, 0);
   if (Page)
   {
    // success
    if (Page->cap_user_time == 1)
    {
     // docs say nanoseconds = (tsc * time_mult) >> time_shift
     //      set nanoseconds = 1000000000 = 1 second in nanoseconds, solve for tsc
     //       =>         tsc = 1000000000 / (time_mult >> time_shift)
     TSCFreq = (1000000000ull << (Page->time_shift / 2)) / (Page->time_mult >> (Page->time_shift - Page->time_shift / 2));
     // If your build configuration supports 128 bit arithmetic, do this:
     // tsc_freq = ((__uint128_t)1000000000ull << (__uint128_t)pc->time_shift) / pc->time_mult;
    }
    munmap(Page, 4096);
   }
   close(fd);
  }
  
  //- Slow path
  if (!TSCFreq)
  {
   u64 OSBegin = OS_ReadOSTimer();
   u64 TSCBegin = OS_ReadCPUTimer();

   // 10ms gives ~4.5 digits of precision - the longer you sleep, the more precise you get
   usleep(10000);
   
   u64 OSEnd = OS_ReadOSTimer();
   u64 TSCEnd = OS_ReadCPUTimer();

   u64 OSFreq = OS_OSTimerFreq();
   TSCFreq = (TSCEnd - TSCBegin) * OSFreq / (OSEnd - OSBegin);
  }
  
  //- Failure case
  if (!TSCFreq)
  {
   TSCFreq = 1000000000;
  }
 }

 return TSCFreq;
}

internal inline u64
OS_ReadOSTimer(void)
{
 u64 Result = 0;
 struct timespec t;
 if (!clock_gettime(CLOCK_MONOTONIC_RAW, &t))
 {
  Result = Cast(u64)t.tv_sec * 1000000000ull + t.tv_nsec;
 }
 return Result;
}

internal inline u64
OS_OSTimerFreq(void)
{
 return 1000000000;
}

internal inline u32
OS_ProcCount(void)
{
 int Procs = get_nprocs();
 u32 Result = SafeCastU32(Procs);
 return Result;
}

internal inline u32
OS_PageSize(void)
{
 int PageSize = getpagesize();
 u32 Result = SafeCastU32(PageSize);
 return Result;
}

internal u32
OS_ThreadGetID(void)
{
 pid_t ThreadID = gettid();
 u32 Result = SafeCastU32(ThreadID);
 return Result;
}