#include <windows.h>
#include <stdint.h>
#include <stdio.h>

int main(int ArgCount, char *Argv[])
{
   uint64_t A = __rdtsc();
#if 1
   system(Argv[1]);
#else
   STARTUPINFOA StartupInfo = {sizeof(StartupInfo)};
   PROCESS_INFORMATION ProcessInfo;
   DWORD CreationFlags = 0;
   CreateProcessA(0, Argv[1], 0, 0, 0, CreationFlags, 0, 0, &StartupInfo, &ProcessInfo);
   WaitForSingleObject(ProcessInfo.hProcess, INFINITE);
#endif
   uint64_t B = __rdtsc();
   printf("%llu\n", B-A);
   
   return 0;
}