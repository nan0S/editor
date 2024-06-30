#if OS_WINDOWS
# include "editor_windows.cpp"
#elif OS_LINUX
# include "editor_linux.cpp"
#endif

internal u64
ReadCPUTimer(void)
{
   return __rdtsc();
}

internal u64
EstimateCPUFrequency(u64 GuessSampleTimeMs)
{
   u64 OSFreq = ReadOSTimerFrequency();
   u64 OSWaitCount = OSFreq * GuessSampleTimeMs / 1000;
   u64 OSElapsed = 0;
   
   u64 CPUBegin = ReadCPUTimer();
   u64 OSBegin = ReadOSTimer();
   while (OSElapsed < OSWaitCount)
   {
      u64 OSEnd = ReadOSTimer();
      OSElapsed = OSEnd - OSBegin;
   }
   u64 CPUEnd = ReadCPUTimer();
   
   u64 CPUElapsed = CPUEnd - CPUBegin;
   u64 CPUFreq = OSFreq * CPUElapsed / OSElapsed;
   
   return CPUFreq;
}