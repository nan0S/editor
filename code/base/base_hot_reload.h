#ifndef BASE_HOT_RELOAD_H
#define BASE_HOT_RELOAD_H

struct hot_reload_library
{
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

#endif //BASE_HOT_RELOAD_H
