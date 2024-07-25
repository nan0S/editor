internal u64
GetPageSize(void)
{
   SYSTEM_INFO Info;
   GetSystemInfo(&Info);
   return Info.dwPageSize;
}

internal u64
GetProcCount(void)
{
   SYSTEM_INFO SysInfo = {};
   GetSystemInfo(&SysInfo);
   u64 Result = SysInfo.dwNumberOfProcessors;
   
   return Result;
}

internal void *
OS_AllocVirtualMemory(u64 Capacity, b32 Commit)
{
   DWORD AllocationType = (MEM_RESERVE | (Commit ? MEM_COMMIT : 0));
   DWORD Protect = (Commit ? PAGE_READWRITE : PAGE_NOACCESS);
   void *Result = VirtualAlloc(0, Capacity, AllocationType, Protect);
   
   return Result;
}

internal void
OS_DeallocVirtualMemory(void *Memory, u64 Size)
{
   VirtualFree(Memory, 0, MEM_RELEASE);
}

internal void
OS_CommitVirtualMemory(void *Memory, u64 Size)
{
   u64 PageSnappedSize = AlignPow2(Size, GetPageSize());
   VirtualAlloc(Memory, PageSnappedSize, MEM_COMMIT, PAGE_READWRITE);
}

internal u64
ReadOSTimerFrequency(void)
{
	LARGE_INTEGER Freq;
	QueryPerformanceFrequency(&Freq);
	return Freq.QuadPart;
}

internal u64
ReadOSTimer(void)
{
   LARGE_INTEGER Counter;
   QueryPerformanceCounter(&Counter);
   return Counter.QuadPart;
}

internal file
OS_OpenFile(string Path, file_access_flags Access)
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

internal void
OS_CloseFile(file File)
{
   CloseHandle(File);
}

typedef BOOL file_op_func(HANDLE       hFile,
                          LPCVOID      lpBuffer,
                          DWORD        nNumberOfBytesToWrite,
                          LPDWORD      lpNumberOfBytesWritten,
                          LPOVERLAPPED lpOverlapped);

internal u64
OS_FileOperation(file_op_func *Op, file File, char *Buf, u64 Target, u64 Offset)
{
   u64 Processed = 0;
   
   u64 Left = Target;
   char *At = Buf;
   u64 OffsetAt = Offset;
   while (Left)
   {
      u64 ToProcess = Min(Left, U32_MAX);
      
      OVERLAPPED Overlapped = {};
      Overlapped.Offset = OffsetAt;
      Overlapped.OffsetHigh = OffsetAt >> 32;
      
      DWORD ActuallyProcessed = 0;
      Op(File, At, ToProcess, &ActuallyProcessed, &Overlapped);
      
      Left -= ActuallyProcessed;
      At += ActuallyProcessed;
      OffsetAt += ActuallyProcessed;
      Processed += ActuallyProcessed;
      
      if (ActuallyProcessed != ToProcess)
      {
         break;
      }
   }
   
   return Processed;
}

internal u64 OS_ReadFile(file File, char *Buf, u64 Read, u64 Offset) { return OS_FileOperation(Cast(file_op_func *)ReadFile, File, Buf, Read, Offset); }
internal u64 OS_WriteFile(file File, char *Buf, u64 Write, u64 Offset) { return OS_FileOperation(WriteFile, File, Buf, Write, Offset); }

internal void
OS_DeleteFile(string Path)
{
   temp_arena Temp = TempArena(0);
   string CPath = CStrFromStr(Temp.Arena, Path);
   DeleteFileA(CPath.Data);
   EndTemp(Temp);
}

internal b32
OS_MoveFile(string Src, string Dest)
{
   temp_arena Temp = TempArena(0);
   string CSrc  = CStrFromStr(Temp.Arena, Src);
   string CDest = CStrFromStr(Temp.Arena, Dest);
   b32 Success = (MoveFileA(CSrc.Data, CDest.Data) != 0);
   EndTemp(Temp);
   
   return Success;
}

internal b32
OS_CopyFile(string Src, string Dest)
{
   temp_arena Temp = TempArena(0);
   string CSrc  = CStrFromStr(Temp.Arena, Src);
   string CDest = CStrFromStr(Temp.Arena, Dest);
   b32 Success = (CopyFileA(CSrc.Data, CDest.Data, 0) != 0);
   EndTemp(Temp);
   
   return Success;
}

internal u64
OS_FileSize(file File)
{
   u64 Result = 0;
   LARGE_INTEGER FileSize = {};
   if (GetFileSizeEx(File, &FileSize))
   {
      Result = FileSize.QuadPart;
   }
   return Result;
}

internal date_time
Win32SysTimeToDateTime(SYSTEMTIME SysTime)
{
   date_time Date = {};
   Date.Ms = SysTime.wMilliseconds;
   Date.Sec = SysTime.wSecond;
   Date.Mins = SysTime.wMinute;
   Date.Hour = SysTime.wHour;
   Date.Day = SysTime.wDay - 1;
   Date.Month = SysTime.wMonth - 1;
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

internal b32
OS_MakeDir(string Path)
{
   temp_arena Temp = TempArena(0);
   string CPath = CStrFromStr(Temp.Arena, Path);
   b32 Success = (CreateDirectoryA(CPath.Data, 0) != 0);
   EndTemp(Temp);
   
   return Success;
}

internal b32
OS_RemoveDir(string Path)
{
   temp_arena Temp = TempArena(0);
   string CPath = CStrFromStr(Temp.Arena, Path);
   b32 Success = (RemoveDirectoryA(CPath.Data) != 0);
   EndTemp(Temp);
   
   return Success;
}

internal b32
OS_ChangeDir(string Path)
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

internal file
StdOutput(void)
{
   HANDLE Std = GetStdHandle(STD_OUTPUT_HANDLE);
   return Std;
}

internal file
StdError(void)
{
   HANDLE Err = GetStdHandle(STD_ERROR_HANDLE);
   return Err;
}

internal void
OutputDebug(string String)
{
   temp_arena Temp = TempArena(0);
   string CString = CStrFromStr(Temp.Arena, String);
   OutputDebugString(CString.Data);
   EndTemp(Temp);
}

internal library
OS_LoadLibrary(char const *Name)
{
   HMODULE Lib = LoadLibraryA(Name);
   return Lib;
}

internal void *
OS_LibraryLoadProc(library Lib, char const *ProcName)
{
   void *Proc = GetProcAddress(Lib, ProcName);
   return Proc;
}

internal void
OS_UnloadLibrary(library Lib)
{
   FreeLibrary(Lib);
}

internal process
OS_LaunchProcess(string_list CmdList)
{
   temp_arena Temp = TempArena(0);
   
   process Handle = {};
   Handle.StartupInfo.cb = SizeOf(Handle.StartupInfo);
   DWORD CreationFlags = 0;
   string Cmd = StrListJoin(Temp.Arena, &CmdList, StrLit(" "));
   CreateProcessA(0, Cmd.Data, 0, 0, 0, CreationFlags, 0, 0,
                  &Handle.StartupInfo, &Handle.ProcessInfo);
   EndTemp(Temp);
   
   return Handle;
}

internal b32
OS_WaitForProcessToFinish(process Process)
{
   b32 Success = (WaitForSingleObject(Process.ProcessInfo.hProcess, INFINITE) == WAIT_OBJECT_0);
   return Success;
}

internal void
OS_CleanupAfterProcess(process Handle)
{
   CloseHandle(Handle.StartupInfo.hStdInput);
   CloseHandle(Handle.StartupInfo.hStdOutput);
   CloseHandle(Handle.StartupInfo.hStdError);
   CloseHandle(Handle.ProcessInfo.hProcess);
   CloseHandle(Handle.ProcessInfo.hThread);
}

internal thread
OS_LaunchThread(thread_func *Func, void *Data)
{
   HANDLE Handle = CreateThread(0, 0, Func, Data, 0, 0);
   return Handle;
}

internal void
OS_WaitThread(thread Thread)
{
   WaitForSingleObject(Thread, INFINITE);
}

internal void
OS_ReleaseThread(thread Thread)
{
   CloseHandle(Thread);
}

internal inline void InitMutex(mutex *Mutex) { InitializeCriticalSection(Mutex); }
internal inline void LockMutex(mutex *Mutex) { EnterCriticalSection(Mutex); }
internal inline void UnlockMutex(mutex *Mutex) { LeaveCriticalSection(Mutex); }
internal inline void DestroyMutex(mutex *Mutex) { DeleteCriticalSection(Mutex); }

internal inline void InitSemaphore(semaphore *Sem, u64 InitialCount, u64 MaxCount) { *Sem = CreateSemaphoreA(0, InitialCount, MaxCount, 0); }
internal inline void PostSemaphore(semaphore *Sem) { ReleaseSemaphore(*Sem, 1, 0); }
internal inline void WaitSemaphore(semaphore *Sem) { WaitForSingleObject(*Sem, INFINITE); }
internal inline void DestroySemaphore(semaphore *Sem) { CloseHandle(*Sem); }

internal inline u64 InterlockedIncr(u64 volatile *Value) { return InterlockedIncrement64(Cast(LONG64 volatile *)Value); }
internal inline u64 InterlockedAdd(u64 volatile *Value, u64 Add) { return InterlockedAdd64(Cast(LONG64 volatile *)Value, Add); }
internal inline u64 InterlockedCmpExch(u64 volatile *Value, u64 Cmp, u64 Exch) { return InterlockedCompareExchange64(Cast(LONG64 volatile *)Value, Exch, Cmp); }

internal inline void InitBarrier(barrier *Barrier, u64 ThreadCount) { InitializeSynchronizationBarrier(Barrier, ThreadCount, 0); }
internal inline void DestroyBarrier(barrier *Barrier) { DeleteSynchronizationBarrier(Barrier); }
internal inline void
WaitBarrier(barrier *Barrier)
{
   // NOTE(hbr): Consider using [SYNCHRONIZATION_BARRIER_FLAGS_NO_DELETE] to improve performance when barrier is never deleted
   EnterSynchronizationBarrier(Barrier, SYNCHRONIZATION_BARRIER_FLAGS_BLOCK_ONLY);
}
