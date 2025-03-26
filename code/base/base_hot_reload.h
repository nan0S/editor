#ifndef BASE_HOT_RELOAD_H
#define BASE_HOT_RELOAD_H

struct hot_reload_library
{
 b32 IsValid;
 
 os_library_handle Library;
 timestamp64 LoadedModifyTime;
 
 string LibraryPath;
 string UsedTempLibraryPath;
 string FreeTempLibraryPath;
 
 u32 FunctionCount;
 void **FunctionTable;
 void **TempFunctionTable;
 char const **FunctionNames;
};

typedef b32 reloaded_b32;

internal hot_reload_library MakeHotReloadableLibrary(arena *Arena, string LibraryPath, char const **FunctionNames, void **FunctionTable, void **TempFunctionTable, u32 FunctionCount);
internal reloaded_b32 HotReloadIfOutOfSync(hot_reload_library *Code);

#endif //BASE_HOT_RELOAD_H
