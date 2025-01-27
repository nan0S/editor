#if OS_WINDOWS
# include "base_os_win32.cpp"
#elif OS_LINUX
# include "base_os_linux.cpp"
#else
# error unsupported OS
#endif

global string GlobalExecutableDirPath;

internal void
OS_Init(int ArgCount, char *Args[])
{
 string ExecutablePath = StrFromCStr(Args[0]);
 GlobalExecutableDirPath = PathChopLastPart(ExecutablePath);
}

internal string
OS_ReadEntireFile(arena *Arena, string Path)
{
 os_file_handle File = OS_FileOpen(Path, FileAccess_Read);
 u64 Size = OS_FileSize(File);
 char *Data = PushArrayNonZero(Arena, Size, char);
 u64 Read = OS_FileRead(File, Data, Size);
 string Result = MakeStr(Data, Read);
 OS_FileClose(File);
 return Result;
}

internal b32
OS_WriteDataToFile(string Path, string Data)
{
 os_file_handle File = OS_FileOpen(Path, FileAccess_Write);
 u64 Written = OS_FileWrite(File, Data.Data, Data.Count);
 b32 Success = (Written == Data.Count);
 OS_FileClose(File);
 
 return Success;
}

internal b32
OS_WriteDataListToFile(string Path, string_list DataList)
{
 os_file_handle File = OS_FileOpen(Path, FileAccess_Write);
 b32 Success = true;
 u64 Offset = 0;
 ListIter(Node, DataList.Head, string_list_node)
 {
  string Data = Node->Str;
  u64 Written = OS_FileWrite(File, Data.Data, Data.Count, Offset);
  Offset += Written;
  if (Written != Data.Count)
  {
   Success = false;
   break;
  }
 }
 OS_FileClose(File);
 
 return Success;
}

inline internal void
OS_PrintFile(os_file_handle File, string Str)
{
 OS_FileWrite(File, Str.Data, Str.Count);
}

inline internal void
OS_Print(string Str)
{
 OS_PrintFile(OS_StdOut(), Str);
}

inline internal void
OS_PrintError(string Str)
{
 OS_PrintFile(OS_StdError(), Str);
}

inline internal void
OS_PrintF(char const *Format, ...)
{
 va_list Args;
 va_start(Args, Format);
 OS_PrintFV(Format, Args);
 va_end(Args);
}

inline internal void
OS_PrintErrorF(char const *Format, ...)
{
 va_list Args;
 va_start(Args, Format);
 OS_PrintErrorFV(Format, Args);
 va_end(Args);
}

inline internal void
OS_PrintDebugF(char const *Format, ...)
{
 va_list Args;
 va_start(Args, Format);
 OS_PrintDebugFV(Format, Args);
 va_end(Args);
}

inline internal void
OS_PrintFileF(os_file_handle File, char const *Format, ...)
{
 va_list Args;
 va_start(Args, Format);
 OS_PrintFileFV(File, Format, Args);
 va_end(Args);
}

inline internal void
OS_PrintFV(char const *Format, va_list Args)
{
 temp_arena Temp = TempArena(0);
 string Str = StrFV(Temp.Arena, Format, Args);
 OS_Print(Str);
 EndTemp(Temp);
}

inline internal void
OS_PrintErrorFV(char const *Format, va_list Args)
{
 temp_arena Temp = TempArena(0);
 string Str = StrFV(Temp.Arena, Format, Args);
 OS_Print(Str);
 EndTemp(Temp);
}

inline internal void
OS_PrintDebugFV(char const *Format, va_list Args)
{
 temp_arena Temp = TempArena(0);
 string Str = StrFV(Temp.Arena, Format, Args);
 OS_PrintDebug(Str);
 EndTemp(Temp);
}

inline internal void
OS_PrintFileFV(os_file_handle File, char const *Format, va_list Args)
{
 temp_arena Temp = TempArena(0);
 string Str = StrFV(Temp.Arena, Format, Args);
 OS_PrintFile(File, Str);
 EndTemp(Temp);
}

internal string
OS_ExecutableRelativeToFullPath(arena *Arena, string Relative)
{
 string Absolute = PathConcat(Arena, GlobalExecutableDirPath, Relative);
 string Result = OS_FullPathFromPath(Arena, Absolute);
 return Result;
}

internal int
OS_CombineExitCodes(int CodeA, int CodeB)
{
 int Result = 0;
 if (CodeA) Result = CodeA;
 else Result = CodeB;
 return Result;
}