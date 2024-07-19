#if OS_WINDOWS
# include "editor_win32.cpp"
#elif OS_LINUX
# include "editor_linux.cpp"
#endif

internal u64
ReadCPUTimer(void)
{
   return __rdtsc();
}

internal u64
EstimateCPUFrequency(u64 GuessSampleTimeMs)
{
   u64 OSFreq = ReadOSTimerFrequency();
   u64 OSWaitCount = OSFreq * GuessSampleTimeMs / 1000;
   u64 OSElapsed = 0;
   
   u64 CPUBegin = ReadCPUTimer();
   u64 OSBegin = ReadOSTimer();
   while (OSElapsed < OSWaitCount)
   {
      u64 OSEnd = ReadOSTimer();
      OSElapsed = OSEnd - OSBegin;
   }
   u64 CPUEnd = ReadCPUTimer();
   
   u64 CPUElapsed = CPUEnd - CPUBegin;
   u64 CPUFreq = OSFreq * CPUElapsed / OSElapsed;
   
   return CPUFreq;
}

internal string
OS_ReadEntireFile(arena *Arena, string Path)
{
   file_handle File = OS_OpenFile(Path, FileAccess_Read);
   u64 Size = OS_FileSize(File);
   char *Data = PushArrayNonZero(Arena, Size, char);
   u64 Read = OS_ReadFile(File, Data, Size);
   string Result = Str(Data, Read);
   OS_CloseFile(File);
   
   return Result;
}

internal b32
OS_WriteDataToFile(string Path, string Data)
{
   file_handle File = OS_OpenFile(Path, FileAccess_Write);
   u64 Written = OS_WriteFile(File, Data.Data, Data.Count);
   b32 Success = (Written == Data.Count);
   OS_CloseFile(File);
   
   return Success;
}

internal b32
OS_WriteDataListToFile(string Path, string_list DataList)
{
   file_handle File = OS_OpenFile(Path, FileAccess_Write);
   b32 Success = true;
   u64 Offset = 0;
   ListIter(Node, DataList.Head, string_list_node)
   {
      string Data = Node->String;
      u64 Written = OS_WriteFile(File, Data.Data, Data.Count, Offset);
      Offset += Written;
      if (Written != Data.Count)
      {
         Success = false;
         break;
      }
   }
   OS_CloseFile(File);
   
   return Success;
}

internal void OutputFile(file_handle Out, string String) { OS_WriteFile(Out, String.Data, String.Count); }
internal void Output(string String) { OutputFile(StdOut(), String); }
internal void OutputFV(char const *Format, va_list Args) { OutputFileFV(StdOut(), Format, Args); }
internal void OutputError(string String) { OutputFile(StdErr(), String); }
internal void OutputErrorFV(char const *Format, va_list Args) { OutputFileFV(StdErr(), Format, Args); }
internal void OutputFile(file_handle Out, char const *String) { OutputFile(Out, StrFromCStr(String)); }
internal void Output(char const *String) { Output(StrFromCStr(String)); }
internal void OutputError(char const *String) { OutputError(StrFromCStr(String)); }

internal void
OutputFileF(file_handle Out, char const *Format, ...)
{
   va_list Args;
   va_start(Args, Format);
   OutputFileFV(Out, Format, Args);
   va_end(Args);
}

internal void
OutputFileFV(file_handle Out, char const *Format, va_list Args)
{
   temp_arena Temp = TempArena(0);
   string String = StrFV(Temp.Arena, Format, Args);
   OutputFile(Out, String);
   EndTemp(Temp);
}

internal void
OutputF(char const *Format, ...)
{
   va_list Args;
   va_start(Args, Format);
   OutputFileFV(StdOut(), Format, Args);
   va_end(Args);
}

internal void
OutputErrorF(char const *Format, ...)
{
   va_list Args;
   va_start(Args, Format);
   OutputFileFV(StdErr(), Format, Args);
   va_end(Args);
}
