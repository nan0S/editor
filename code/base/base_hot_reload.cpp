internal hot_reload_library
MakeHotReloadableLibrary(arena *Arena, string LibraryPath, char const **FunctionNames,
                         void **FunctionTable, void **TempFunctionTable, u32 FunctionCount)
{
 hot_reload_library Result = {};
 
 string BaseLibraryPath = StrChopLastDot(LibraryPath);
 string LibraryExt = StrAfterLastDot(LibraryPath);
 string TempName1 = StrF(Arena, "%S_temp.%S", BaseLibraryPath, LibraryExt);
 string TempName2 = StrF(Arena, "%S_temp2.%S", BaseLibraryPath, LibraryExt);
 
 Result.LibraryPath = LibraryPath;
 Result.UsedTempLibraryPath = TempName2;
 Result.FreeTempLibraryPath = TempName1;
 
 Result.FunctionNames = FunctionNames;
 Result.FunctionTable = FunctionTable;
 Result.TempFunctionTable = TempFunctionTable;
 Result.FunctionCount = FunctionCount;
 
 return Result;
}

internal b32
HotReloadIfOutOfSync(hot_reload_library *Code)
{
 b32 Reloaded = false;
 
 file_attrs DLLAttrs = OS_FileAttributes(Code->LibraryPath);
 if (DLLAttrs.ModifyTime > Code->LoadedModifyTime)
 {
  if (OS_FileCopy(Code->LibraryPath, Code->FreeTempLibraryPath))
  {
   os_library_handle NewLibrary = OS_LibraryLoad(Code->FreeTempLibraryPath);
   if (NewLibrary)
   {
    b32 NewLibraryIsValid = true;
    for (u32 FunctionIndex = 0;
         FunctionIndex < Code->FunctionCount;
         ++FunctionIndex)
    {
     char const *FunctionName = Code->FunctionNames[FunctionIndex];
     void *LoadedFunction = OS_LibraryProc(NewLibrary, FunctionName);
     if (LoadedFunction)
     {
      Code->TempFunctionTable[FunctionIndex] = LoadedFunction;
     }
     else
     {
      NewLibraryIsValid = false;
      break;
     }
    }
    
    if (NewLibraryIsValid)
    {
     os_library_handle OldLibrary = Code->Library;
     if (OldLibrary)
     {
      OS_LibraryUnload(OldLibrary);
     }
     
     Code->Library = NewLibrary;
     Code->IsValid = true;
     Code->LoadedModifyTime = DLLAttrs.ModifyTime;
     
     ArrayCopy(Code->FunctionTable, Code->TempFunctionTable, Code->FunctionCount);
     Swap(Code->UsedTempLibraryPath, Code->FreeTempLibraryPath, string);
     
     // TODO(hbr): maybe remove this
     OS_PrintDebugF("%S reloaded\n", Code->LibraryPath);
     
     Reloaded = true;
    }
    else
    {
     OS_LibraryUnload(NewLibrary);
    }
   }
  }
 }
 
 return Reloaded;
}