/* ========================================================================
   Parametric Curve Editor
   Master's Thesis by Hubert Obrzut
   Supervisor: Paweł Woźny
   University of Wrocław
   Faculty of Mathematics and Computer Science
   Institute of Computer Science
   Date: September 2025
   ======================================================================== */

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

internal string
PlatformKeyCombinationToString(arena *Arena,
                               platform_key Key,
                               platform_key_modifier_flags Modifiers)
{
 temp_arena Temp = TempArena(Arena);
 
 string_list Strs = {};
 ForEachEnumVal(Modifier, PlatformKeyModifier_Count, platform_key_modifier)
 {
  if (Modifiers & (1 << Modifier))
  {
   StrListPush(Temp.Arena, &Strs, PlatformKeyModifierNames[Modifier]);
  }
 }
 StrListPush(Temp.Arena, &Strs, PlatformKeyNames[Key]);
 
 string_list_join_options Opts = {};
 Opts.Sep = StrLit("+");
 string Result = StrListJoin(Arena, &Strs, Opts);
 
 return Result;
}

internal b32
IsKeyPress(platform_event *Event, platform_key Key, platform_key_modifier_flags Modifiers)
{
 b32 Match = ((Event->Type == PlatformEvent_Press) &&
              (Event->Key == Key) &&
              ((Modifiers == AnyKeyModifier) || ((Event->Modifiers & AnyKeyModifier) == Modifiers)));
 return Match;
}

internal b32
IsKeyRelease(platform_event *Event, platform_key Key)
{
 b32 Match = ((Event->Type == PlatformEvent_Release) &&
              (Event->Key == Key));
 return Match;
}
