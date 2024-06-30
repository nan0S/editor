function void *
VirtualMemoryReserve(u64 Capacity)
{
   void *Result = mmap(0, Capacity, PROT_NONE, MAP_ANON|MAP_PRIVATE, -1, 0);
   return Result;
}

function void
VirtualMemoryCommit(void *Memory, u64 Size)
{
   mprotect(Memory, Size, PROT_READ|PROT_WRITE);
}

function void
VirtualMemoryRelease(void *Memory, u64 Size)
{
   munmap(Memory, Size);
}

function u64
ReadOSTimerFrequency(void)
{
   return 1000000;
}

function u64
ReadOSTimer(void)
{
   struct timeval Time;
   
   gettimeofday(&Time, 0);
   
   u64 OSFreq = ReadOSTimerFrequency();
   u64 Result = OSFreq * Time.tv_sec + Time.tv_usec;
   
   return Result;
}
