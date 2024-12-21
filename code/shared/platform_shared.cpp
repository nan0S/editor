internal platform_event *
PushPlatformEvent(shared_platform_input *Input, platform_event_type Type, platform_event_flags Flags)
{
 platform_event *Result = 0;
 if (Input->EventCount < SHARED_PLATFORM_MAX_EVENT_COUNT)
 {
  Result = GlobalWin32Input.Events + GlobalWin32Input.EventCount++;
  Result->Type = Type;
  Result->Flags = Win32GetEventFlags();
 }
 return Result;
}
