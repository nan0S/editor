internal void *
OS_Reserve(u64 Reserve, b32 Commit)
{
 int ProtFlags = (Commit ? (PROT_READ | PROT_WRITE) : PROT_NONE);
 void *Result = mmap(0, Reserve, ProtFlags, MAP_ANON | MAP_PRIVATE, -1, 0);
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

internal void
OS_Decommit(void *Memory, u64 Size)
{
 mprotect(Memory, Size, PROT_NONE);
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
 if (Access & FileAccess_Write) Flags |= (O_CREAT | O_TRUNC);
 int File = open(CPath.Data, Flags, 0644);
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

internal void
OS_PrintDebug(string Str)
{
 OS_PrintErrorF("[DEBUG]: %S", Str);
}

internal b32
OS_FileDelete(string Path)
{
 temp_arena Temp = TempArena(0);
 string CPath = CStrFromStr(Temp.Arena, Path);
 int Ret = unlink(CPath.Data);
 b32 Success = (Ret == 0);
 EndTemp(Temp);
 return Success;
}

internal b32
OS_FileMove(string Src, string Dst)
{
 temp_arena Temp = TempArena(0);
 string CSrc = CStrFromStr(Temp.Arena, Src);
 string CDst = CStrFromStr(Temp.Arena, Dst);
 int Ret = rename(CSrc.Data, CDst.Data);
 b32 Success = (Ret == 0);
 EndTemp(Temp);
 return Success;
}

internal b32
OS_FileValid(os_file_handle File)
{
 b32 Valid = (File != -1);
 return Valid;
}

internal b32
OS_FileCopy(string Src, string Dst)
{
 os_file_handle SrcFile = OS_FileOpen(Src, FileAccess_Read);
 u64 SrcFileSize = OS_FileSize(SrcFile);
 os_file_handle DstFile = OS_FileOpen(Dst, FileAccess_Write);
 
 int Ret = sendfile(DstFile, SrcFile, 0, SrcFileSize);
 b32 Success = (Ret == 0);
 
 OS_FileClose(DstFile);
 OS_FileClose(SrcFile);
 
 return Success;
}

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

internal b32
OS_DirMake(string Path)
{
 temp_arena Temp = TempArena(0);
 string CPath = CStrFromStr(Temp.Arena, Path);
 int Ret = mkdir(CPath.Data, 0755);
 b32 Success = (Ret == 0);
 EndTemp(Temp);
 return Success;
}

internal b32
OS_DirRemove(string Path)
{
 temp_arena Temp = TempArena(0);
 string CPath = CStrFromStr(Temp.Arena, Path);
 int Ret = rmdir(CPath.Data);
 b32 Success = (Ret == 0);
 EndTemp(Temp);
 return Success;
}

internal b32
OS_DirChange(string Path)
{
 temp_arena Temp = TempArena(0);
 string CPath = CStrFromStr(Temp.Arena, Path);
 int Ret = chdir(CPath.Data);
 b32 Success = (Ret == 0);
 EndTemp(Temp);
 return Success;
}

internal string
OS_CurrentDir(arena *Arena)
{
 string Result = {};
 temp_arena Temp = BeginTemp(Arena);
 
 u32 Count = PATH_MAX;
 b32 Extracting = true;
 do
 {
  char *Buffer = PushArrayNonZero(Arena, Count, char);
  char *Ret = getcwd(Buffer, Count);
  if (Ret == 0)
  {
   if (errno == ERANGE) // buffer too short, try again
   {
    EndTemp(Temp);
    Count <<= 1;
   }
   else // unknown error, return empty string
   {
    Extracting = false;
   }
  }
  else // successfully extracted current dir, return it
  {
   Result = StrFromCStr(Buffer);
   Extracting = false;
  }
 } while (Extracting);
 
 return Result;
}

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

internal os_library_handle
OS_LibraryLoad(string Path)
{
 temp_arena Temp = TempArena(0);
 string CPath = CStrFromStr(Temp.Arena, Path);
 void *Result = dlopen(CPath.Data, RTLD_LAZY);
 EndTemp(Temp);
 return Result;
}

internal void *
OS_LibraryProc(os_library_handle Lib, char const *ProcName)
{
 void *Result = dlsym(Lib, ProcName);
 return Result;
}

internal void
OS_LibraryUnload(os_library_handle Lib)
{
 int Ret = dlclose(Lib);
 Assert(Ret == 0);
}

internal os_process_handle
OS_ProcessLaunch(string_list CmdList)
{
 os_process_handle Result = 0;
 temp_arena Temp = TempArena(0);
 
 if (CmdList.NodeCount > 0)
 {
  string ExePath = CStrFromStr(Temp.Arena, CmdList.Head->Str);
  
  char **Argv = PushArray(Temp.Arena, CmdList.NodeCount + 1, char *);
  string_list_node *Node = CmdList.Head;
  for (u32 ArgIndex = 0;
       ArgIndex < CmdList.NodeCount;
       ++ArgIndex)
  {
   string Arg = Node->Str;
   string CArg = CStrFromStr(Temp.Arena, Arg);
   Argv[ArgIndex] = CArg.Data;
   Node = Node->Next;
  }
  
  pid_t Pid = fork();
  if (Pid >= 0)
  {
   if (Pid == 0) // child case
   {
    int Ret = execvp(ExePath.Data, Argv);
    // NOTE(hbr): execve should never return, if it does
    // it's always -1, and something is wrong, exit immediately
    // because we are already in child process
    Assert(Ret == -1);
    exit(EXIT_FAILURE);
   }
   else // parent case
   {
    Result = Pid;
   }
  }
 }
 
 EndTemp(Temp);
 
 return Result;
}

internal int
OS_ProcessWait(os_process_handle Process)
{
 int Status = 0;
 pid_t Ret = waitpid(Process, &Status, 0);
 // TODO(hbr): uncomment or actually do sth better - check if the process handle is invalid - if so do nothing - do this always in different functions around here as well 
 //Assert(Ret == Process);
 int ExitCode = WEXITSTATUS(Status);
 return ExitCode;
}

internal os_thread_handle
OS_ThreadLaunch(os_thread_func *Func, void *Data)
{
 pthread_t Thread = {};
 int Ret = pthread_create(&Thread, 0, Func, Data);
 Assert(Ret == 0);
 return Thread;
}

internal void
OS_ThreadWait(os_thread_handle Thread)
{
 int Ret = pthread_join(Thread, 0);
 Assert(Ret == 0);
}

internal void
OS_ThreadRelease(os_thread_handle Thread)
{
 // TODO(hbr): not sure actually whether we should call pthread_detach here.
 // For sure it could fail if it was called after OS_ThreadWait, but also API
 // suggests that we should release thread after launching it, so we should
 // do something at least. I'm fine with detach here always and maybe failing
 // but who cares if this fails (there is OS_ThreadRelease API call because of Win32
 // thread API).
 pthread_detach(Thread);
}

internal void
OS_MutexAlloc(os_mutex_handle *Mutex)
{
 int Ret = pthread_mutex_init(Mutex, 0);
 Assert(Ret == 0);
}

internal void
OS_MutexLock(os_mutex_handle *Mutex)
{
 int Ret = pthread_mutex_lock(Mutex);
 Assert(Ret == 0);
}

internal void
OS_MutexUnlock(os_mutex_handle *Mutex)
{
 int Ret = pthread_mutex_unlock(Mutex);
 Assert(Ret == 0);
}

internal void
OS_MutexDealloc(os_mutex_handle *Mutex)
{
 int Ret = pthread_mutex_destroy(Mutex);
 Assert(Ret == 0);
}

internal void
OS_SemaphoreAlloc(os_semaphore_handle *Sem, u32 InitialCount, u32 MaxCount)
{
 int Ret = sem_init(Sem, 0, InitialCount);
 Assert(Ret == 0);
}

internal void
OS_SemaphorePost(os_semaphore_handle *Sem)
{
 int Ret = sem_post(Sem);
 Assert(Ret == 0);
}

internal void
OS_SemaphoreWait(os_semaphore_handle *Sem)
{
 int Ret = sem_wait(Sem);
 Assert(Ret == 0);
}

internal void
OS_SemaphoreDealloc(os_semaphore_handle *Sem)
{
 int Ret = sem_destroy(Sem);
 Assert(Ret == 0);
}

internal void
OS_BarrierAlloc(os_barrier_handle *Barrier, u32 ThreadCount)
{
 int Ret = pthread_barrier_init(Barrier, 0, ThreadCount);
 Assert(Ret == 0);
}

internal void
OS_BarrierWait(os_barrier_handle *Barrier)
{
 pthread_barrier_wait(Barrier);
}

internal void
OS_BarrierDealloc(os_barrier_handle *Barrier)
{
 int Ret = pthread_barrier_destroy(Barrier);
 Assert(Ret == 0);
}

internal inline u64
OS_AtomicIncr64(u64 volatile *Value)
{
 u64 Result = OS_AtomicAdd64(Value, 1); // NOTE(hbr): I don't think Linux has builtin increment function
 return Result;
}

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
 local u64 TSC_Freq = 0;
 if (!TSC_Freq)
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
  int FileDesc = syscall(298, &PE, 0, -1, -1, 0);
  if (FileDesc != -1)
  { 
   struct perf_event_mmap_page *Page = Cast(struct perf_event_mmap_page *)mmap(NULL, 4096, PROT_READ, MAP_SHARED, FileDesc, 0);
   if (Page)
   {
    // success
    if (Page->cap_user_time == 1)
    {
     // docs say nanoseconds = (tsc * time_mult) >> time_shift
     //      set nanoseconds = 1000000000 = 1 second in nanoseconds, solve for tsc
     //       =>         tsc = 1000000000 / (time_mult >> time_shift)
     TSC_Freq = (1000000000ull << (Page->time_shift / 2)) / (Page->time_mult >> (Page->time_shift - Page->time_shift / 2));
     // If your build configuration supports 128 bit arithmetic, do this:
     // tsc_freq = ((__uint128_t)1000000000ull << (__uint128_t)pc->time_shift) / pc->time_mult;
    }
    munmap(Page, 4096);
   }
   close(FileDesc);
  }
  
  //- Slow path
  if (!TSC_Freq)
  {
   u64 OSFreq = OS_OSTimerFreq();
   u64 WaitMilliseconds = 10;
   u64 OSTargetDuration = OSFreq * WaitMilliseconds / 1000;
   
   u64 OSEnd = 0, OSElapsed = 0;
   u64 OSBegin = OS_ReadOSTimer();
   u64 OSTargetEnd = OSBegin + OSTargetDuration;
   u64 TSCBegin = OS_ReadCPUTimer();
   
   do {
    OSEnd = OS_ReadOSTimer();
   } while (OSEnd < OSTargetEnd);   
   u64 TSCEnd = OS_ReadCPUTimer();
   
   TSC_Freq = (TSCEnd - TSCBegin) * OSFreq / (OSEnd - OSBegin);
  }
  
  //- Failure case
  if (!TSC_Freq)
  {
   TSC_Freq = 1000000000;
  }
 }
 
 return TSC_Fre;
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

internal void
OS_Sleep(u64 Milliseconds)
{
 usleep(Milliseconds * 1000);
}