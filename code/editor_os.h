#ifndef EDITOR_OS_H
#define EDITOR_OS_H

#if OS_WINDOWS
# include "editor_windows.h"
#elif OS_LINUX
# include "editor_linux.h"
#else
# error OS layer not implemented
#endif

function void *VirtualMemoryReserve(u64 Capacity);
function void VirtualMemoryCommit(void *Memory, u64 Size);
function void VirtualMemoryRelease(void *Memory, u64 Size);

function u64 ReadOSTimerFrequency(void);
function u64 ReadOSTimer(void);
function u64 ReadCPUTimer(void);
function u64 EstimateCPUFrequency(u64 GuessSampleTimeMs);

//function file_handle OS_OpenFile(string Path);
//function void OS_CloseFile(file_handle File);
//function u64 OS_ReadFile(file_handle File, void *Buf, u64 Read, u64 Offset = 0);

#endif //EDITOR_OS_H
