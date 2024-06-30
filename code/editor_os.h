#ifndef EDITOR_OS_H
#define EDITOR_OS_H

#if OS_WINDOWS
# include "editor_windows.h"
#elif OS_LINUX
# include "editor_linux.h"
#else
# error OS layer not implemented
#endif

internal void *VirtualMemoryReserve(u64 Capacity);
internal void VirtualMemoryCommit(void *Memory, u64 Size);
internal void VirtualMemoryRelease(void *Memory, u64 Size);

internal u64 ReadOSTimerFrequency(void);
internal u64 ReadOSTimer(void);
internal u64 ReadCPUTimer(void);
internal u64 EstimateCPUFrequency(u64 GuessSampleTimeMs);

//internal file_handle OS_OpenFile(string Path);
//internal void OS_CloseFile(file_handle File);
//internal u64 OS_ReadFile(file_handle File, void *Buf, u64 Read, u64 Offset = 0);

#endif //EDITOR_OS_H
