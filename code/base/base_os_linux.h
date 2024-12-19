#ifndef BASE_OS_LINUX_H
#define BASE_OS_LINUX_H

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <pthread.h>
#include <dirent.h>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <linux/perf_event.h>

typedef int os_file_handle;
typedef void *os_library_handle;
typedef pid_t os_process_handle;
typedef pthread_t os_thread_handle;
typedef pthread_mutex_t os_mutex_handle;
typedef sem_t os_semaphore_handle;
typedef pthread_barrier_t os_barrier_handle;

#define OS_THREAD_FUNC(Name) void *Name(void *ThreadEntryDataPtr)

struct dir_iter
{
 DIR *Dir;
 b32 NotFirstTime;
};

#endif //BASE_OS_LINUX_H