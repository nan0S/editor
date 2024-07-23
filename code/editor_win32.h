#ifndef EDITOR_WINDOWS_H
#define EDITOR_WINDOWS_H

#include <windows.h>
#include <intrin.h>

typedef HANDLE file_handle;
typedef HMODULE library_handle;

struct process_handle
{
   STARTUPINFOA StartupInfo;
   PROCESS_INFORMATION ProcessInfo;
};

typedef CRITICAL_SECTION mutex_handle;
typedef HANDLE semaphore_handle;
typedef SYNCHRONIZATION_BARRIER barrier_handle;

typedef HANDLE thread_handle;
typedef DWORD WINAPI thread_func(LPVOID lpParameter);

#define OS_THREAD_FUNC(Name) DWORD WINAPI Name(LPVOID Data)

#endif //EDITOR_WINDOWS_H
