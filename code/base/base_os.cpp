#if OS_WINDOWS
# include "base_os_win32.cpp"
#elif OS_LINUX
# include "base_os_linux.cpp"
#else
# error unsupported OS
#endif

internal void
OS_EntryPoint(int ArgCount, char *Argv[])
{
 string ProgramInvocationPathRel = StrFromCStr(Argv[0]);
 string ProgramInvocationDir = PathChopLastPart(ProgramInvocationPathRel);
 b32 Success = OS_DirChange(ProgramInvocationDir);
 Assert(Success);
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