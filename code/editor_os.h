#ifndef EDITOR_OS_H
#define EDITOR_OS_H

#if OS_WINDOWS
#include "editor_windows.h"
#elif OS_LINUX
#include "editor_linux.h"
#else
#error OS layer not implemented
#endif

//~ Virtual Memory
function void *VirtualMemoryReserve(u64 Capacity);
function void VirtualMemoryCommit(void *Memory, u64 Size);
function void VirtualMemoryRelease(void *Memory, u64 Size);

//~ Timing
function u64 ReadOSTimerFrequency(void);
function u64 ReadOSTimer(void);

function u64 ReadCPUTimer(void);
function u64 EstimateCPUFrequency(u64 GuessSampleTimeMs);

//~ File Handling
#if 0
struct file_handle;

enum file_access_flags
{
   FileAccessFlags_Read = (1<<0),
   FileAccessFlags_Write = (1<<1),
};

function file_handle OpenFile(arena *Arena, string FilePath, file_access_flags FileAccessFlags, error_string *OutError);
function error_string CloseFile(arena *Arena, file_handle FileHandle);
// TODO(hbr): Change to generic read
function file_contents ReadWholeFile(arena *Arena, file_handle FileHandle, error_string *OutError);
function error_string WriteFile(arena *Arena, file_handle FileHandle, string Content);
#endif

#endif //EDITOR_OS_H
