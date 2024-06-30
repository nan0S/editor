internal u64
PageSize(void)
{
   SYSTEM_INFO Info;
   GetSystemInfo(&Info);
   return Info.dwPageSize;
}

internal void *
VirtualMemoryReserve(u64 Capacity)
{
   u64 GbSnappedCapacity = Capacity;
   GbSnappedCapacity += Gigabytes(1) - 1;
   GbSnappedCapacity -= GbSnappedCapacity % Gigabytes(1);
   void *Result = VirtualAlloc(0, GbSnappedCapacity, MEM_RESERVE, PAGE_NOACCESS);
   
   return Result;
}

internal void
VirtualMemoryRelease(void *Memory, u64 Size)
{
   VirtualFree(Memory, 0, MEM_RELEASE);
}

internal void
VirtualMemoryCommit(void *Memory, u64 Size)
{
   u64 PageSnappedSize = Size;
   PageSnappedSize += PageSize() - 1;
   PageSnappedSize -= PageSnappedSize % PageSize();
   VirtualAlloc(Memory, PageSnappedSize, MEM_COMMIT, PAGE_READWRITE);
}

internal u64
ReadOSTimerFrequency(void)
{
	LARGE_INTEGER Freq;
	QueryPerformanceFrequency(&Freq);
	return Freq.QuadPart;
}

internal u64
ReadOSTimer(void)
{
   LARGE_INTEGER Counter;
   QueryPerformanceCounter(&Counter);
   return Counter.QuadPart;
}

#if 0
internal u64
OS_OpenFile(string Path)
{
   temp_arena Temp = TempArena(0);
   
   
   EndTemp(Temp);
}
#endif