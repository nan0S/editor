#ifndef EDITOR_WINDOWS_H
#define EDITOR_WINDOWS_H

#include <windows.h>
#include <intrin.h>

typedef HANDLE file;
typedef HMODULE library;

struct process
{
   STARTUPINFOA StartupInfo;
   PROCESS_INFORMATION ProcessInfo;
};

typedef CRITICAL_SECTION mutex;
typedef HANDLE semaphore;
typedef SYNCHRONIZATION_BARRIER barrier;

typedef HANDLE thread;
typedef DWORD WINAPI thread_func(LPVOID lpParameter);

#define OS_THREAD_FUNC(Name) DWORD WINAPI Name(LPVOID Data)

struct dir_iter
{
   b32 NotFirstTime;
   HANDLE Handle;
};

#endif //EDITOR_WINDOWS_H
