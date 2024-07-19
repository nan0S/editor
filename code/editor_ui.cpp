internal b32
UI_Combo_(u8 *InOutEnum, u8 EnumCount, char const *EnumNames[], string Label)
{
   temp_arena Temp = TempArena(0);
   string CLabel = CStrFromStr(Temp.Arena, Label);
   int InOutEnum_ = *InOutEnum;
   b32 Result = Cast(b32)ImGui::Combo(CLabel.Data, &InOutEnum_, EnumNames, EnumCount);
   *InOutEnum = InOutEnum_;
   EndTemp(Temp);
   
   return Result;
}

internal b32
UI_ComboF_(u8 *InOutEnum, u8 EnumCount, char const *EnumNames[], char const *Format, ...)
{
   temp_arena Temp = TempArena(0);
   va_list  Args;
   va_start(Args, Format);
   string Label = StrFV(Temp.Arena, Format, Args);
   b32 Result = UI_Combo_(InOutEnum, EnumCount, EnumNames, Label);
   va_end(Args);
   EndTemp(Temp);
   
   return Result;
}

internal void
UI_Checkbox(b32 *InOutEnabled, string Label)
{
   temp_arena Temp = TempArena(0);
   
   string CLabel = CStrFromStr(Temp.Arena, Label);
   bool InOutEnabled_ = *InOutEnabled;
   ImGui::Checkbox(CLabel.Data, &InOutEnabled_);
   *InOutEnabled = Cast(b32)InOutEnabled_;
   EndTemp(Temp);
}

internal void
UI_CheckboxF(b32 *InOutEnabled, char const *Format, ...)
{
   temp_arena Temp = TempArena(0);
   va_list Args;
   va_start(Args, Format);
   string Label = StrFV(Temp.Arena, Format, Args);
   UI_Checkbox(InOutEnabled, Label);
   va_end(Args);
   EndTemp(Temp);
}

internal b32
UI_ColorPicker(color *InOutColor, string Label)
{
   temp_arena Temp = TempArena(0);
   string CLabel = CStrFromStr(Temp.Arena, Label);
   f32 ColorF32[4] = {
      InOutColor->R / 255.0f,
      InOutColor->G / 255.0f,
      InOutColor->B / 255.0f,
      InOutColor->A / 255.0f,
   };
   b32 Result = Cast(b32)ImGui::ColorEdit4(CLabel.Data, ColorF32);
   *InOutColor = ColorMake(Cast(u8)(255 * ColorF32[0]),
                           Cast(u8)(255 * ColorF32[1]),
                           Cast(u8)(255 * ColorF32[2]),
                           Cast(u8)(255 * ColorF32[3]));
   EndTemp(Temp);
   
   return Result;
}

internal b32
UI_ColorPickerF(color *InOutColor, char const *Format, ...)
{
   temp_arena Temp = TempArena(0);
   va_list Args;
   va_start(Args, Format);
   string Label = StrFV(Temp.Arena, Format, Args);
   b32 Result = UI_ColorPicker(InOutColor, Label);
   va_end(Args);
   EndTemp(Temp);
   
   return Result;
}

internal b32
UI_Button(string Label)
{
   temp_arena Temp = TempArena(0);
   string CLabel = CStrFromStr(Temp.Arena, Label);
   b32 Result = Cast(b32)ImGui::Button(CLabel.Data);
   EndTemp(Temp);
   
   return Result;
}

internal b32
UI_ButtonF(char const *Format, ...)
{
   temp_arena Temp = TempArena(0);
   va_list Args;
   va_start(Args, Format);
   string Label = StrFV(Temp.Arena, Format, Args);
   b32 Result = UI_Button(Label);
   va_end(Args);
   EndTemp(Temp);
   
   return Result;
}

internal void
UI_AngleSlider(rotation_2d *InOutRotation, string Label)
{
   temp_arena Temp = TempArena(0);
   string CLabel = CStrFromStr(Temp.Arena, Label);
   f32 RotationRad = Rotation2DToRadians(*InOutRotation);
   ImGui::SliderAngle(CLabel.Data, &RotationRad);
   *InOutRotation = Rotation2DFromRadians(RotationRad);
   EndTemp(Temp);
}

internal void
UI_AngleSliderF(rotation_2d *InOutRotation, char const *Format, ...)
{
   temp_arena Temp = TempArena(0);
   va_list Args;
   va_start(Args, Format);
   string Label = StrFV(Temp.Arena, Format, Args);
   UI_AngleSlider(InOutRotation, Label);
   va_end(Args);
   EndTemp(Temp);
}

internal string
UI_InputText(char *Buf, u64 BufSize, string Label)
{
   temp_arena Temp = TempArena(0);
   string CLabel = CStrFromStr(Temp.Arena, Label);
   ImGui::InputText(CLabel.Data, Buf, BufSize);
   string Result = StrFromCStr(Buf);
   EndTemp(Temp);
   
   return Result;
}

internal string
UI_InputTextF(char *Buf, u64 BufSize, char const *Format, ...)
{
   temp_arena Temp = TempArena(0);
   va_list Args;
   va_start(Args, Format);
   string Label = StrFV(Temp.Arena, Format, Args);
   string Result = UI_InputText(Buf, BufSize, Label);
   va_end(Args);
   EndTemp(Temp);
   
   return Result;
}

internal void
UI_PushLabel(string Label)
{
   temp_arena Temp = TempArena(0);
   string CLabel = CStrFromStr(Temp.Arena, Label);
   ImGui::PushID(CLabel.Data);
   EndTemp(Temp);
}

internal void
UI_PushLabelF(char const *Format, ...)
{
   temp_arena Temp = TempArena(0);
   va_list Args;
   va_start(Args, Format);
   string Label = StrFV(Temp.Arena, Format, Args);
   UI_PushLabel(Label);
   va_end(Args);
   EndTemp(Temp);
}

internal void UI_PushId(u64 Id) { ImGui::PushID(Id); }
internal void UI_PopId(void) { ImGui::PopID(); }

#define UI_Label(Label)        DeferBlock(UI_PushLabel(Label), UI_PopId())
#define UI_LabelF(Format, ...) DeferBlock(UI_PushLabelF(Label), UI_PopId())
#define UI_Id(Id)              DeferBlock(UI_PushId(Id), UI_PopId())

internal b32
UI_DragFloat(f32 *InOutValue, f32 MinValue, f32 MaxValue, char const *ValueFormat, string Label)
{
   temp_arena Temp = TempArena(0);
   string CLabel = CStrFromStr(Temp.Arena, Label);
   f32 CurrentVal = Abs(*InOutValue);
   f32 Speed = PowF32(CurrentVal, 0.7f) / 100.0f;
   Speed = ClampBot(0.001f, Speed);
   char const *Format = ValueFormat;
   if (Format == 0) Format = "%.3f";
   b32 Result = Cast(b32)ImGui::DragFloat(CLabel.Data, InOutValue, Speed, MinValue,
                                          MaxValue, Format, ImGuiSliderFlags_NoRoundToFormat);
   EndTemp(Temp);
   
   return Result;
}

internal b32
UI_DragFloatF(f32 *InOutValue, f32 MinValue, f32 MaxValue, char const *ValueFormat, char const *Format, ...)
{
   temp_arena Temp = TempArena(0);
   va_list Args;
   va_start(Args, Format);
   string Label = StrFV(Temp.Arena, Format, Args);
   b32 Result = UI_DragFloat(InOutValue, MinValue, MaxValue, ValueFormat, Label);
   va_end(Args);
   EndTemp(Temp);
   
   return Result;
}

internal b32
UI_DragFloat2(f32 InOutValues[2], f32 MinValue, f32 MaxValue, char const *ValueFormat, string Label)
{
   temp_arena Temp = TempArena(0);
   string CLabel = CStrFromStr(Temp.Arena, Label);
   f32 CurrentVal = (Abs(InOutValues[0]) + Abs(InOutValues[1])) / 2;
   f32 Speed = PowF32(CurrentVal, 0.7f) / 100.0f;
   Speed = ClampBot(0.001f, Speed);
   char const *Format = ValueFormat;
   if (Format == 0) Format = "%.3f";
   b32 Result = Cast(b32)ImGui::DragFloat2(CLabel.Data, InOutValues, Speed, MinValue,
                                           MaxValue, Format, ImGuiSliderFlags_NoRoundToFormat);
   EndTemp(Temp);
   
   return Result;
}

internal b32
UI_DragFloat2F(f32 InOutValues[2], f32 MinValue, f32 MaxValue, char const *ValueFormat, char const *Format, ...)
{
   temp_arena Temp = TempArena(0);
   va_list Args;
   va_start(Args, Format);
   string Label = StrFV(Temp.Arena, Format, Args);
   b32 Result = UI_DragFloat2(InOutValues, MinValue, MaxValue, ValueFormat, Label);
   va_end(Args);
   EndTemp(Temp);
   
   return Result;
}

internal void UI_NewRow(void) { ImGui::NewLine(); }
internal void UI_SameRow(void) { ImGui::SameLine(); }

internal void
UI_SeparatorText(string Text)
{
   temp_arena Temp = TempArena(0);
   string CText = CStrFromStr(Temp.Arena, Text);
   ImGui::SeparatorText(CText.Data);
   EndTemp(Temp);
}

internal void
UI_SeparatorTextF(char const *Format, ...)
{
   temp_arena Temp = TempArena(0);
   va_list Args;
   va_start(Args, Format);
   string Text = StrFV(Temp.Arena, Format, Args);
   UI_SeparatorText(Text);
   EndTemp(Temp);
   va_end(Args);
}

internal int
SafeCastInt(s64 Value)
{
   AssertAlways(INT_MIN <= Value && Value <= INT_MAX);
   int Result = Cast(int)Value;
   return Result;
}

internal b32
UI_SliderInteger_(s64 *Value, s64 MinValue, s64 MaxValue, string Label)
{
   temp_arena Temp = TempArena(0);
   string CLabel = CStrFromStr(Temp.Arena, Label);
   int ValueInt = SafeCastInt(*Value);
   b32 Result = Cast(b32)ImGui::SliderInt(CLabel.Data, &ValueInt, SafeCastInt(MinValue), SafeCastInt(MaxValue));
   *Value = ValueInt;
   EndTemp(Temp);
   
   return Result;
}

internal b32
UI_SliderIntegerF_(s64 *Value, s64 MinValue, s64 MaxValue, char const *Format, ...)
{
   va_list Args;
   va_start(Args, Format);
   temp_arena Temp = TempArena(0);
   string Label = StrFV(Temp.Arena, Format, Args);
   b32 Result = UI_SliderInteger(Value, MinValue, MaxValue, Label);
   EndTemp(Temp);
   va_end(Args);
   
   return Result;
}

internal void
UI_Text(string Text)
{
   temp_arena Temp = TempArena(0);
   string CText = CStrFromStr(Temp.Arena, Text);
   ImGui::Text(CText.Data);
   EndTemp(Temp);
}

internal void
UI_TextF(char const *Format, ...)
{
   va_list Args;
   va_start(Args, Format);
   temp_arena Temp = TempArena(0);
   string Text = StrFV(Temp.Arena, Format, Args);
   UI_Text(Text);
   EndTemp(Temp);
   va_end(Args);
}