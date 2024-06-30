internal void *
VirtualMemoryReserve(u64 Capacity)
{
   void *Result = mmap(0, Capacity, PROT_NONE, MAP_ANON|MAP_PRIVATE, -1, 0);
   return Result;
}

internal void
VirtualMemoryCommit(void *Memory, u64 Size)
{
   mprotect(Memory, Size, PROT_READ|PROT_WRITE);
}

internal void
VirtualMemoryRelease(void *Memory, u64 Size)
{
   munmap(Memory, Size);
}

internal u64
ReadOSTimerFrequency(void)
{
   return 1000000;
}

internal u64
ReadOSTimer(void)
{
   struct timeval Time;
   
   gettimeofday(&Time, 0);
   
   u64 OSFreq = ReadOSTimerFrequency();
   u64 Result = OSFreq * Time.tv_sec + Time.tv_usec;
   
   return Result;
}
