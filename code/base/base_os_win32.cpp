internal void *
OS_Reserve(u64 Reserve, b32 Commit)
{
 DWORD AllocationType = (MEM_RESERVE | (Commit ? MEM_COMMIT : 0));
 void *Result = VirtualAlloc(0, Reserve, AllocationType, PAGE_READWRITE);
 return Result;
}

internal void
OS_Release(void *Memory, u64 Size)
{
 VirtualFree(Memory, 0, MEM_RELEASE);
}

internal void
OS_Commit(void *Memory, u64 Size)
{
 VirtualAlloc(Memory, Size, MEM_COMMIT, PAGE_READWRITE);
}

internal void
OS_Decommit(void *Memory, u64 Size)
{
 VirtualAlloc(Memory, Size, MEM_RESET, PAGE_NOACCESS);
}

internal date_time
Win32SysTimeToDateTime(SYSTEMTIME SysTime)
{
 date_time Date = {};
 Date.Ms = SysTime.wMilliseconds;
 Date.Sec = SafeCastU8(SysTime.wSecond);
 Date.Mins = SafeCastU8(SysTime.wMinute);
 Date.Hour = SafeCastU8(SysTime.wHour);
 Date.Day = SafeCastU8(SysTime.wDay - 1);
 Date.Month = SafeCastU8(SysTime.wMonth - 1);
 Date.Year = SysTime.wYear;
 
 return Date;
}

internal timestamp64
Win32FileTimeToTimestamp(FILETIME FileTime)
{
 SYSTEMTIME SysTime = {};
 FileTimeToSystemTime(&FileTime, &SysTime);
 date_time DateTime = Win32SysTimeToDateTime(SysTime);
 timestamp64 Ts = DateTimeToTimestamp(DateTime);
 
 return Ts;
}

internal b32
OS_IterDir(string Path, dir_iter *Iter, dir_entry *OutEntry)
{
 b32 Found = false;
 
 b32 Looking = true;
 while (Looking)
 {
  WIN32_FIND_DATAA Win32FindData = {};
  b32 HasNext = false;
  if (Iter->NotFirstTime)
  {
   HasNext = Cast(b32)FindNextFileA(Iter->Handle, &Win32FindData);
  }
  else
  {
   temp_arena Temp = TempArena(0);
   string PathWithWild = StrF(Temp.Arena, "%S\\*", Path);
   Iter->Handle = FindFirstFileA(PathWithWild.Data, &Win32FindData);
   HasNext = (Iter->Handle != INVALID_HANDLE_VALUE);
   Iter->NotFirstTime = true;
   EndTemp(Temp);
  }
  
  if (HasNext)
  {
   string EntryName = StrFromCStr(Win32FindData.cFileName);
   if (StrEqual(EntryName, StrLit(".")) || StrEqual(EntryName, StrLit("..")))
   {
    // NOTE(hbr): Skip
   }
   else
   {
    OutEntry->FileName = StrFromCStr(Win32FindData.cFileName);
    OutEntry->Attrs.FileSize = (((Cast(u64)Win32FindData.nFileSizeHigh) << 32) | Win32FindData.nFileSizeLow);
    OutEntry->Attrs.CreateTime = Win32FileTimeToTimestamp(Win32FindData.ftCreationTime);
    OutEntry->Attrs.ModifyTime = Win32FileTimeToTimestamp(Win32FindData.ftLastWriteTime);
    OutEntry->Attrs.Dir = (Win32FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
    
    Found = true;
    Looking = false;
   }
  }
  else
  {
   Looking = false;
  }
 }
 
 if (!Found)
 {
  CloseHandle(Iter->Handle);
 }
 
 return Found;
}

internal os_file_handle
OS_FileOpen(string Path, file_access_flags Access)
{
 temp_arena Temp = TempArena(0);
 
 string CPath = CStrFromStr(Temp.Arena, Path);
 
 DWORD DesiredAccess = 0;
 if (Access & FileAccess_Read) DesiredAccess |= GENERIC_READ;
 if (Access & FileAccess_Write) DesiredAccess |= GENERIC_WRITE;
 
 DWORD CreationDisposition = OPEN_EXISTING;
 if (Access & FileAccess_Write) CreationDisposition = CREATE_ALWAYS;
 
 HANDLE Result = CreateFileA(CPath.Data, DesiredAccess, FILE_SHARE_READ, 0,
                             CreationDisposition, FILE_ATTRIBUTE_NORMAL, 0);
 
 EndTemp(Temp);
 
 return Result;
}

internal b32
OS_FileValid(os_file_handle File)
{
 b32 Valid = (File != INVALID_HANDLE_VALUE);
 return Valid;
}

internal void
OS_FileClose(os_file_handle File)
{
 CloseHandle(File);
}

typedef BOOL win32_file_op_func(HANDLE       hFile,
                                LPCVOID      lpBuffer,
                                DWORD        nNumberOfBytesToWrite,
                                LPDWORD      lpNumberOfBytesWritten,
                                LPOVERLAPPED lpOverlapped);

internal u64
OS_FileOperation(win32_file_op_func *Op, os_file_handle File, char *Buf, u64 Target, u64 Offset)
{
 u64 Processed = 0;
 
 OVERLAPPED Overlapped_ = {};
 Overlapped_.Offset = Cast(u32)(Offset >> 0);
 Overlapped_.OffsetHigh = Cast(u32)(Offset >> 32);
 
 OVERLAPPED *Overlapped = (Offset == U64_MAX ? 0 : &Overlapped_);
 
 u64 Left = Target;
 char *At = Buf;
 while (Left)
 {
  u32 ToProcess = SafeCastU32(ClampTop(Left, U32_MAX));
  
  DWORD ActuallyProcessed = 0;
  Op(File, At, ToProcess, &ActuallyProcessed, Overlapped);
  
  Left -= ActuallyProcessed;
  At += ActuallyProcessed;
  Processed += ActuallyProcessed;
  
  if (ActuallyProcessed != ToProcess)
  {
   break;
  }
  
  Overlapped = 0;
 }
 
 return Processed;
}

internal u64 OS_FileRead(os_file_handle File, char *Buf, u64 Read, u64 Offset) { return OS_FileOperation(Cast(win32_file_op_func *)ReadFile, File, Buf, Read, Offset); }
internal u64 OS_FileWrite(os_file_handle File, char *Buf, u64 Write, u64 Offset) { return OS_FileOperation(WriteFile, File, Buf, Write, Offset); }

internal u64
OS_FileSize(os_file_handle File)
{
 u64 Result = 0;
 LARGE_INTEGER FileSize = {};
 if (GetFileSizeEx(File, &FileSize))
 {
  Result = FileSize.QuadPart;
 }
 return Result;
}

internal file_attrs
OS_FileAttributes(string Path)
{
 file_attrs Result = {};
 temp_arena Temp = TempArena(0);
 string CPath = CStrFromStr(Temp.Arena, Path);
 WIN32_FILE_ATTRIBUTE_DATA Attrs = {};
 if (GetFileAttributesExA(CPath.Data, GetFileExInfoStandard, &Attrs))
 {
  Result.FileSize = (((Cast(u64)Attrs.nFileSizeHigh) << 32) | Attrs.nFileSizeLow);
  Result.CreateTime = Win32FileTimeToTimestamp(Attrs.ftCreationTime);
  Result.ModifyTime = Win32FileTimeToTimestamp(Attrs.ftLastWriteTime);
  Result.Dir = (Attrs.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
 }
 EndTemp(Temp);
 
 return Result;
}

inline internal os_file_handle
OS_StdOut(void)
{
 HANDLE Std = GetStdHandle(STD_OUTPUT_HANDLE);
 return Std;
}

inline internal os_file_handle
OS_StdError(void)
{
 HANDLE Err = GetStdHandle(STD_ERROR_HANDLE);
 return Err;
}

inline internal void
OS_PrintDebug(string Str)
{
 temp_arena Temp = TempArena(0);
 string CStr = CStrFromStr(Temp.Arena, Str);
 OutputDebugString(CStr.Data);
 EndTemp(Temp);
}

internal b32
OS_FileDelete(string Path)
{
 temp_arena Temp = TempArena(0);
 string CPath = CStrFromStr(Temp.Arena, Path);
 BOOL Ret = DeleteFileA(CPath.Data);
 b32 Success = (Ret != 0);
 EndTemp(Temp);
 return Success;
}

internal b32
OS_FileMove(string Src, string Dst)
{
 temp_arena Temp = TempArena(0);
 string CSrc  = CStrFromStr(Temp.Arena, Src);
 string CDst = CStrFromStr(Temp.Arena, Dst);
 b32 Success = (MoveFileA(CSrc.Data, CDst.Data) != 0);
 EndTemp(Temp);
 
 return Success;
}

internal b32
OS_FileCopy(string Src, string Dst)
{
 temp_arena Temp = TempArena(0);
 string CSrc  = CStrFromStr(Temp.Arena, Src);
 string CDst = CStrFromStr(Temp.Arena, Dst);
 b32 Success = (CopyFileA(CSrc.Data, CDst.Data, 0) != 0);
 EndTemp(Temp);
 
 return Success;
}

internal b32
OS_FileExists(string Path)
{
 temp_arena Temp = TempArena(0);
 string CPath = CStrFromStr(Temp.Arena, Path);
 DWORD Attributes = GetFileAttributesA(CPath.Data);
 b32 Result = (Attributes != INVALID_FILE_ATTRIBUTES) && !(Attributes & FILE_ATTRIBUTE_DIRECTORY);
 EndTemp(Temp);
 
 return Result;
}

internal b32
OS_DirMake(string Path)
{
 temp_arena Temp = TempArena(0);
 string CPath = CStrFromStr(Temp.Arena, Path);
 b32 Success = (CreateDirectoryA(CPath.Data, 0) != 0);
 EndTemp(Temp);
 
 return Success;
}

internal b32
OS_DirRemove(string Path)
{
 temp_arena Temp = TempArena(0);
 string CPath = CStrFromStr(Temp.Arena, Path);
 b32 Success = (RemoveDirectoryA(CPath.Data) != 0);
 EndTemp(Temp);
 
 return Success;
}

internal b32
OS_DirChange(string Path)
{
 temp_arena Temp = TempArena(0);
 string CPath = CStrFromStr(Temp.Arena, Path);
 b32 Success = (SetCurrentDirectoryA(CPath.Data) != 0);
 EndTemp(Temp);
 
 return Success;
}

internal string
OS_CurrentDir(arena *Arena)
{
 string Result = {};
 DWORD RequiredSize = GetCurrentDirectory(0, 0);
 Result.Data = PushArrayNonZero(Arena, RequiredSize, char);
 Result.Count = GetCurrentDirectory(RequiredSize, Result.Data);
 Assert(Result.Count == RequiredSize - 1);
 
 return Result;
}

internal string
OS_FullPathFromPath(arena *Arena, string Path)
{
 string Result = {};
 
 temp_arena Temp = TempArena(Arena);
 string CPath = CStrFromStr(Temp.Arena, Path);
 DWORD BufferLength = MAX_PATH + 1;
 Result.Data = PushArrayNonZero(Arena, BufferLength, char);
 Result.Count = GetFullPathNameA(CPath.Data, BufferLength, Result.Data, 0);
 if (Result.Count < BufferLength)
 {
  Result.Data[Result.Count] = 0;
 }
 EndTemp(Temp);
 
 return Result;
}

internal os_library_handle
OS_LibraryLoad(string Path)
{
 temp_arena Temp = TempArena(0);
 string CPath = CStrFromStr(Temp.Arena, Path);
 HMODULE Lib = LoadLibraryA(CPath.Data);
 EndTemp(Temp);
 return Lib;
}

internal void *
OS_LibraryProc(os_library_handle Lib, char const *ProcName)
{
 void *Proc = Cast(void *)GetProcAddress(Lib, ProcName);
 return Proc;
}

internal void
OS_LibraryUnload(os_library_handle Lib)
{
 FreeLibrary(Lib);
}

internal os_process_handle
OS_ProcessLaunch(string_list CmdList)
{
 temp_arena Temp = TempArena(0);
 
 os_process_handle Handle = {};
 Handle.StartupInfo.cb = SizeOf(Handle.StartupInfo);
 DWORD CreationFlags = 0;
 
 string_list_join_options Opts = {};
 Opts.Sep = StrLit(" ");
 string Cmd = StrListJoin(Temp.Arena, &CmdList, Opts);
 
 CreateProcessA(0, Cmd.Data, 0, 0, 0, CreationFlags, 0, 0,
                &Handle.StartupInfo, &Handle.ProcessInfo);
 
 EndTemp(Temp);
 
 return Handle;
}

internal int
OS_ProcessWait(os_process_handle Process)
{
 {
  DWORD Ret = (WaitForSingleObject(Process.ProcessInfo.hProcess, INFINITE) == WAIT_OBJECT_0);
  b32 Success = (Ret != WAIT_FAILED);
  Assert(Success);
 }
 
#if 0
 DWORD ExitCode = 0;
 {
  BOOL Success = GetExitCodeProcess(Process.ProcessInfo.hProcess, &ExitCode);
  Assert(Success);
 }
 
 CloseHandle(Process.StartupInfo.hStdInput);
 CloseHandle(Process.StartupInfo.hStdOutput);
 CloseHandle(Process.StartupInfo.hStdError);
 CloseHandle(Process.ProcessInfo.hProcess);
 CloseHandle(Process.ProcessInfo.hThread);
#endif
 DWORD ExitCode = 0;
 
 return ExitCode;
}

internal os_thread_handle
OS_ThreadLaunch(os_thread_func *Func, void *Data)
{
 HANDLE Handle = CreateThread(0, 0, Func, Data, 0, 0);
 return Handle;
}

internal void
OS_ThreadWait(os_thread_handle Thread)
{
 WaitForSingleObject(Thread, INFINITE);
}

internal void
OS_ThreadRelease(os_thread_handle Thread)
{
 CloseHandle(Thread);
}

inline internal void
OS_MutexAlloc(os_mutex_handle *Mutex)
{
 InitializeCriticalSection(Mutex);
}

inline internal void
OS_MutexLock(os_mutex_handle *Mutex)
{
 EnterCriticalSection(Mutex);
}

inline internal void
OS_MutexUnlock(os_mutex_handle *Mutex)
{
 LeaveCriticalSection(Mutex);
}

inline internal void
OS_MutexDealloc(os_mutex_handle *Mutex)
{
 DeleteCriticalSection(Mutex);
}

inline internal void
OS_SemaphoreAlloc(os_semaphore_handle *Sem, u32 InitialCount, u32 MaxCount)
{
 *Sem = CreateSemaphoreA(0, InitialCount, MaxCount, 0);
}

inline internal void
OS_SemaphorePost(os_semaphore_handle *Sem)
{
 ReleaseSemaphore(*Sem, 1, 0);
}

inline internal void
OS_SemaphoreWait(os_semaphore_handle *Sem)
{
 WaitForSingleObject(*Sem, INFINITE);
}

inline internal void
OS_SemaphoreDealloc(os_semaphore_handle *Sem)
{
 CloseHandle(*Sem);
}

inline internal u64
OS_AtomicIncr64(u64 volatile *Value)
{
 return InterlockedIncrement64(Cast(LONG64 volatile *)Value);
}

inline internal u64
OS_AtomicAdd64(u64 volatile *Value, u64 Add)
{
 return InterlockedAdd64(Cast(LONG64 volatile *)Value, Add);
}

inline internal u64
OS_AtomicCmpExch64(u64 volatile *Value, u64 Cmp, u64 Exch)
{
 return InterlockedCompareExchange64(Cast(LONG64 volatile *)Value, Exch, Cmp);
}

inline internal u32
OS_AtomicIncr32(u32 volatile *Value)
{
 return InterlockedIncrement(Cast(LONG volatile *)Value);
}

inline internal u32
OS_AtomicAdd32(u32 volatile *Value, u32 Add)
{
 return InterlockedAdd(Cast(LONG volatile *)Value, Add);
}

inline internal u32
OS_AtomicCmpExch32(u32 volatile *Value, u32 Cmp, u32 Exch)
{
 return InterlockedCompareExchange(Cast(LONG volatile *)Value, Exch, Cmp);
}

inline internal void
OS_BarrierAlloc(os_barrier_handle *Barrier, u32 ThreadCount)
{
 InitializeSynchronizationBarrier(Barrier, ThreadCount, 0);
}

inline internal void
OS_BarrierWait(os_barrier_handle *Barrier)
{
 // NOTE(hbr): Consider using [SYNCHRONIZATION_BARRIER_FLAGS_NO_DELETE] to improve performance when barrier is never deleted
 EnterSynchronizationBarrier(Barrier, SYNCHRONIZATION_BARRIER_FLAGS_BLOCK_ONLY);
}

inline internal void
OS_BarrierDealloc(os_barrier_handle *Barrier)
{
 DeleteSynchronizationBarrier(Barrier);
}

inline internal u64
OS_ReadCPUTimer(void)
{
 return __rdtsc();
}

inline internal u64
OS_CPUTimerFreq(void)
{
 local u64 TSCFreq = 0;
 if (!TSCFreq)
 {
  // Fast path: Load kernel-mapped memory page
  HMODULE NTDLL = LoadLibraryA("ntdll.dll");
  if (NTDLL)
  {
   int (*NtQuerySystemInformation)(int, void *, unsigned int, unsigned int *) =
   (int (*)(int, void *, unsigned int, unsigned int *))GetProcAddress(NTDLL, "NtQuerySystemInformation");
   
   if (NtQuerySystemInformation)
   {
    volatile u64 *HypervisorSharedPage = NULL;
    unsigned int Size = 0;
    
    // SystemHypervisorSharedPageInformation == 0xc5
    int Result = (NtQuerySystemInformation)(0xc5, Cast(void *)&HypervisorSharedPage, SizeOf(HypervisorSharedPage), &Size);
    
    // success
    if (Size == SizeOf(HypervisorSharedPage) && Result >= 0)
    {
     // docs say ReferenceTime = ((VirtualTsc * TscScale) >> 64)
     //      set ReferenceTime = 10000000 = 1 second @ 10MHz, solve for VirtualTsc
     //       =>    VirtualTsc = 10000000 / (TscScale >> 64)
     TSCFreq = (10000000ull << 32) / (HypervisorSharedPage[1] >> 32);
     // If your build configuration supports 128 bit arithmetic, do this:
     // TSCFreq = ((unsigned __int128)10000000ull << (unsigned __int128)64ull) / HypervisorSharedPage[1];
    }
   }
   FreeLibrary(NTDLL);
  }
  
  // Slow path
  if (!TSCFreq)
  {
   // Get time before sleep
   u64 OSBegin = 0;
   QueryPerformanceCounter((LARGE_INTEGER *)&OSBegin);
   u64 TSCBegin = __rdtsc();
   
   Sleep(2);
   
   // Get time after sleep
   u64 OSEnd = 0;
   QueryPerformanceCounter((LARGE_INTEGER *)&OSEnd);
   u64 TSCEnd = __rdtsc();
   
   // Do the math to extrapolate the RDTSC ticks elapsed in 1 second
   u64 OSFreq = 0;
   QueryPerformanceFrequency((LARGE_INTEGER *)&OSFreq);
   TSCFreq = (TSCEnd - TSCBegin) * OSFreq / (OSEnd - OSBegin);
  }
  
  // Failure case
  if (!TSCFreq)
  {
   TSCFreq = 1000000000;
  }
 }
 
 return TSCFreq;
}

inline internal u64
OS_ReadOSTimer(void)
{
 LARGE_INTEGER Counter;
 QueryPerformanceCounter(&Counter);
 return Counter.QuadPart;
}

internal u64
OS_TimerFreq(void)
{
 local u64 Frequency = 0;
 if (Frequency == 0)
 {
  LARGE_INTEGER Freq;
  QueryPerformanceFrequency(&Freq);
  Frequency = Freq.QuadPart;
 }
 return Frequency;
}

inline internal SYSTEM_INFO
Win32GetInfo(void)
{
 local SYSTEM_INFO Info = {};
 local b32 Computed = false;
 if (!Computed)
 {
  GetSystemInfo(&Info);
  Computed = true;
 }
 return Info;
}

internal u32
OS_ProcCount(void)
{
 u32 Result = Win32GetInfo().dwNumberOfProcessors;
 return Result;
}

internal u32
OS_PageSize(void)
{
 u32 Result = Win32GetInfo().dwPageSize;
 return Result;
}

internal u32
OS_ThreadGetID(void)
{
 DWORD ID = GetCurrentThreadId();
 u32 Result = Cast(u32)ID;
 return Result;
}

internal void
OS_Sleep(u64 Milliseconds)
{
 u32 Ms = SafeCastU32(Milliseconds);
 Sleep(Ms);
}

internal string
FileDialogFiltersToWin32Filter(arena *Arena, os_file_dialog_filters Filters)
{
 string_list FilterList = {};
 
 if (Filters.AnyFileFilter)
 {
  StrListPush(Arena, &FilterList, StrLit("All Files (*.*)"));
  StrListPush(Arena, &FilterList, StrLit("*.*"));
 }
 
 for (u32 FilterIndex = 0;
      FilterIndex < Filters.FilterCount;
      ++FilterIndex)
 {
  os_file_dialog_filter Filter = Filters.Filters[FilterIndex];
  
  string_list ExtensionRegexes = {};
  for (u32 ExtensionIndex = 0;
       ExtensionIndex < Filter.ExtensionCount;
       ++ExtensionIndex)
  {
   string Extension = Filter.Extensions[ExtensionIndex];
   StrListPushF(Arena, &ExtensionRegexes, "*.%S", Extension);
  }
  
  string_list_join_options SpaceOpts = {};
  SpaceOpts.Sep = StrLit(" ");
  string SpaceJoinedExtensionRegexes = StrListJoin(Arena, &ExtensionRegexes, SpaceOpts);
  StrListPushF(Arena, &FilterList, "%S (%S)", Filter.DisplayName, SpaceJoinedExtensionRegexes);
  
  string_list_join_options SemicolonOpts = {};
  SemicolonOpts.Sep = StrLit(";");
  string SemicolonJoinedExtensionRegexes = StrListJoin(Arena, &ExtensionRegexes, SemicolonOpts);
  StrListPush(Arena, &FilterList, SemicolonJoinedExtensionRegexes);
 }
 
 string_list_join_options NilOpts = {};
 NilOpts.Sep = StrLit("\0");
 NilOpts.Post = StrLit("\0");
 string Win32Filter = StrListJoin(Arena, &FilterList, NilOpts);
 
 return Win32Filter;
}

internal os_file_dialog_result
OS_OpenFileDialog(arena *Arena, os_file_dialog_filters Filters)
{
 os_file_dialog_result Result = {};
 
 temp_arena Temp = TempArena(Arena);
 
 string Win32Filter = FileDialogFiltersToWin32Filter(Temp.Arena, Filters);
 
 char Buffer[2048];
 OPENFILENAME Open = {};
 Open.lStructSize = SizeOf(Open);
 Open.lpstrFile = Buffer;
 Open.lpstrFile[0] = '\0';
 Open.nMaxFile = ArrayCount(Buffer);
 Open.lpstrFilter = Win32Filter.Data;
 Open.nFilterIndex = 1;
 Open.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER;
 
 if (GetOpenFileName(&Open) == TRUE)
 {
  char *At = Buffer;
  u32 FileCount = 0;
  while (At[0])
  {
   At += CStrLen(At) + 1;
   ++FileCount;
  }
  
  At = Buffer;
  string BasePath = StrCopy(Arena, StrFromCStr(At));
  At += BasePath.Count + 1;
  
  string *FilePaths = PushArrayNonZero(Arena, FileCount, string);
  if (FileCount == 1)
  {
   FilePaths[0] = BasePath;
  }
  else
  {
   Assert(FileCount > 0);
   --FileCount;
   for (u32 FileIndex = 0;
        FileIndex < FileCount;
        ++FileIndex)
   {
    // NOTE(hbr): No need to StrCopy here because we PathConcat anyway
    string FileName = StrFromCStr(At);
    At += FileName.Count + 1;
    FilePaths[FileIndex] = PathConcat(Arena, BasePath, FileName);
   }
  }
  
  Result.FileCount = FileCount;
  Result.FilePaths = FilePaths;
 }
 
 EndTemp(Temp);
 
 return Result;
}

internal void
OS_MessageBox(string Msg)
{             
 temp_arena Temp = TempArena(0);
 string CMsg = CStrFromStr(Temp.Arena, Msg);
 MessageBoxA(0, CMsg.Data, 0, MB_OK);
 EndTemp(Temp);
}