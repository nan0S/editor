internal f32
UseAndExtractDeltaTime(platform_input_output *Input)
{
 f32 dt = Input->dtForFrame;
 Input->RefreshRequested = true;
 return dt;
}

internal f32
ExtractDeltaTimeOnly(platform_input_output *Input)
{
 return Input->dtForFrame;
}

internal b32
IsKeyPress(platform_event *Event, platform_key Key, platform_event_flags Flags)
{
 b32 Match = ((Event->Type == PlatformEvent_Press) &&
              (Event->Key == Key) &&
              ((Flags == AnyKeyModifier) || ((Event->Flags & AnyKeyModifier) == Flags)));
 return Match;
}

internal b32
IsKeyRelease(platform_event *Event, platform_key Key)
{
 b32 Match = ((Event->Type == PlatformEvent_Release) &&
              (Event->Key == Key));
 return Match;
}
