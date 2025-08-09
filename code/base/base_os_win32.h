#ifndef BASE_OS_WIN32_H
#define BASE_OS_WIN32_H

#include <windows.h>
#include <intrin.h>
//#include <Shlobj.h>

typedef HANDLE os_file_handle;
typedef HMODULE os_library_handle;
typedef CRITICAL_SECTION os_mutex_handle;
typedef HANDLE os_semaphore_handle;
typedef SYNCHRONIZATION_BARRIER os_barrier_handle;
typedef HANDLE os_thread_handle;

#define OS_THREAD_FUNC(Name) DWORD WINAPI Name(LPVOID ThreadEntryDataPtr)

struct os_process_handle
{
 STARTUPINFOA StartupInfo;
 PROCESS_INFORMATION ProcessInfo;
};

struct dir_iter
{
 b32 NotFirstTime;
 HANDLE Handle;
};

#endif //BASE_OS_WIN32_H
