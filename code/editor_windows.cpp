internal u64
PageSize(void)
{
   SYSTEM_INFO Info;
   GetSystemInfo(&Info);
   return Info.dwPageSize;
}

internal void *
AllocVirtualMemory(u64 Capacity, b32 Commit)
{
   DWORD AllocationType = (MEM_RESERVE | (Commit ? MEM_COMMIT : 0));
   DWORD Protect = (Commit ? PAGE_READWRITE : PAGE_NOACCESS);
   void *Result = VirtualAlloc(0, Capacity, AllocationType, Protect);
   
   return Result;
}

internal void
DeallocVirtualMemory(void *Memory, u64 Size)
{
   VirtualFree(Memory, 0, MEM_RELEASE);
}

internal void
CommitVirtualMemory(void *Memory, u64 Size)
{
   u64 PageSnappedSize = Size;
   PageSnappedSize += PageSize() - 1;
   PageSnappedSize -= PageSnappedSize % PageSize();
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

internal file_handle
OS_OpenFile(string Path, file_access_flags Access)
{
   temp_arena Temp = TempArena(0);
   
   string CPath = CStrFromStr(Temp.Arena, Path);
   
   DWORD DesiredAccess = 0;
   if (Access & FileAccess_Read) DesiredAccess |= GENERIC_READ;
   if (Access & FileAccess_Write) DesiredAccess |= GENERIC_WRITE;
   
   DWORD CreationDisposition = OPEN_EXISTING;
   if (Access & FileAccess_Write) CreationDisposition = CREATE_ALWAYS;
   
   HANDLE Result = CreateFileA(CPath.Data, DesiredAccess, FILE_SHARE_READ, 0, CreationDisposition, FILE_ATTRIBUTE_NORMAL, 0);
   
   EndTemp(Temp);
   
   return Result;
}

internal void
OS_CloseFile(file_handle File)
{
   CloseHandle(File);
}

typedef BOOL file_op_func(HANDLE       hFile,
                          LPCVOID      lpBuffer,
                          DWORD        nNumberOfBytesToWrite,
                          LPDWORD      lpNumberOfBytesWritten,
                          LPOVERLAPPED lpOverlapped);

internal u64
OS_FileOperation(file_op_func *Op, file_handle File, char *Buf, u64 Target, u64 Offset)
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

internal u64 OS_ReadFile(file_handle File, char *Buf, u64 Read, u64 Offset) { return OS_FileOperation(Cast(file_op_func *)ReadFile, File, Buf, Read, Offset); }
internal u64 OS_WriteFile(file_handle File, char *Buf, u64 Write, u64 Offset) { return OS_FileOperation(WriteFile, File, Buf, Write, Offset); }

internal void
OS_DeleteFile(string Path)
{
   temp_arena Temp = TempArena(0);
   string CPath = CStrFromStr(Temp.Arena, Path);
   DeleteFileA(CPath.Data);
   EndTemp(Temp);
}

internal u64
OS_FileSize(file_handle File)
{
   u64 Result = 0;
   LARGE_INTEGER FileSize = {};
   if (GetFileSizeEx(File, &FileSize))
   {
      Result = FileSize.QuadPart;
   }
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

internal file_handle
StdOut(void)
{
   HANDLE Std = GetStdHandle(STD_OUTPUT_HANDLE);
   return Std;
}


internal file_handle
StdErr(void)
{
   HANDLE Err = GetStdHandle(STD_ERROR_HANDLE);
   return Err;
}

internal library_handle
OS_LoadLibrary(char const *Name)
{
   HMODULE Lib = LoadLibraryA(Name);
   return Lib;
}

internal void *
OS_LibraryLoadProc(library_handle Lib, char const *ProcName)
{
   void *Proc = GetProcAddress(Lib, ProcName);
   return Proc;
}

internal void
OS_UnloadLibrary(library_handle Lib)
{
   FreeLibrary(Lib);
}