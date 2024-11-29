#ifndef WIN32_SHARED_H
#define WIN32_SHARED_H

#define WIN32_BEGIN_DEBUG_BLOCK(Name) u64 Name##BeginTSC = OS_ReadCPUTimer()
#define WIN32_END_DEBUG_BLOCK(Name) do { \
u64 EndTSC = OS_ReadCPUTimer(); \
f32 Elapsed = Win32SecondsElapsed(Name##BeginTSC, EndTSC); \
OS_PrintDebugF("%s: %fms\n", #Name, Elapsed * 1000.0f); \
} while(0)
internal f32
Win32SecondsElapsed(u64 BeginTSC, u64 EndTSC)
{
 u64 ElapsedTSC = EndTSC - BeginTSC;
 f32 Result = Cast(f32)ElapsedTSC / OS_CPUTimerFreq();
 return Result;
}

#endif //WIN32_SHARED_H
