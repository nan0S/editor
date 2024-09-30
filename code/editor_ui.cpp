internal b32
UI_Combo_(u8 *Enum, u8 EnumCount, char const *EnumNames[], string Label)
{
   temp_arena Temp = TempArena(0);
   string CLabel = CStrFromStr(Temp.Arena, Label);
   int Enum_ = *Enum;
   b32 Result = Cast(b32)ImGui::Combo(CLabel.Data, &Enum_, EnumNames, EnumCount);
   *Enum = Enum_;
   EndTemp(Temp);
   
   return Result;
}

internal b32
UI_ComboF_(u8 *Enum, u8 EnumCount, char const *EnumNames[], char const *Format, ...)
{
   temp_arena Temp = TempArena(0);
   va_list  Args;
   va_start(Args, Format);
   string Label = StrFV(Temp.Arena, Format, Args);
   b32 Result = UI_Combo_(Enum, EnumCount, EnumNames, Label);
   va_end(Args);
   EndTemp(Temp);
   
   return Result;
}

internal b32
UI_Checkbox(b32 *Enabled, string Label)
{
   temp_arena Temp = TempArena(0);
   string CLabel = CStrFromStr(Temp.Arena, Label);
   bool Enabled_ = *Enabled;
   b32 Result = Cast(b32)ImGui::Checkbox(CLabel.Data, &Enabled_);
   *Enabled = Cast(b32)Enabled_;
   EndTemp(Temp);
   
   return Result;
}

internal b32
UI_CheckboxF(b32 *Enabled, char const *Format, ...)
{
   temp_arena Temp = TempArena(0);
   va_list Args;
   va_start(Args, Format);
   string Label = StrFV(Temp.Arena, Format, Args);
   b32 Result = UI_Checkbox(Enabled, Label);
   va_end(Args);
   EndTemp(Temp);
   
   return Result;
}

internal b32
UI_ColorPicker(color *Color, string Label)
{
   temp_arena Temp = TempArena(0);
   string CLabel = CStrFromStr(Temp.Arena, Label);
   f32 ColorF32[4] = {
      Color->R / 255.0f,
      Color->G / 255.0f,
      Color->B / 255.0f,
      Color->A / 255.0f,
   };
   b32 Result = Cast(b32)ImGui::ColorEdit4(CLabel.Data, ColorF32);
   *Color = MakeColor(Cast(u8)(255 * ColorF32[0]),
                      Cast(u8)(255 * ColorF32[1]),
                      Cast(u8)(255 * ColorF32[2]),
                      Cast(u8)(255 * ColorF32[3]));
   EndTemp(Temp);
   
   return Result;
}

internal b32
UI_ColorPickerF(color *Color, char const *Format, ...)
{
   temp_arena Temp = TempArena(0);
   va_list Args;
   va_start(Args, Format);
   string Label = StrFV(Temp.Arena, Format, Args);
   b32 Result = UI_ColorPicker(Color, Label);
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
UI_AngleSlider(rotation_2d *Rotation, string Label)
{
   temp_arena Temp = TempArena(0);
   string CLabel = CStrFromStr(Temp.Arena, Label);
   f32 RotationRad = Rotation2DToRadians(*Rotation);
   ImGui::SliderAngle(CLabel.Data, &RotationRad);
   *Rotation = Rotation2DFromRadians(RotationRad);
   EndTemp(Temp);
}

internal void
UI_AngleSliderF(rotation_2d *Rotation, char const *Format, ...)
{
   temp_arena Temp = TempArena(0);
   va_list Args;
   va_start(Args, Format);
   string Label = StrFV(Temp.Arena, Format, Args);
   UI_AngleSlider(Rotation, Label);
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
internal void UI_PopLabel(void) { UI_PopId(); }
internal void UI_BeginDisabled(b32 Disabled) { ImGui::BeginDisabled(Cast(bool)Disabled); }
internal void UI_EndDisabled(void) { ImGui::EndDisabled(); }

internal void
UI_PushTextColor(color Color)
{
   ImVec4 ImColor = ImVec4(Color.R, Color.G, Color.B, Color.A);
   ImGui::PushStyleColor(ImGuiCol_Text, ImColor);
}

internal void UI_PopTextColor(void) { ImGui::PopStyleColor(1); }

internal void
UI_PushAlpha(f32 Alpha)
{
   ImGui::PushStyleVar(ImGuiStyleVar_Alpha, Alpha);
}

internal void
UI_PopAlpha(void)
{
   ImGui::PopStyleVar();
}

internal b32
UI_DragFloat(f32 *Value, f32 MinValue, f32 MaxValue, char const *ValueFormat, string Label)
{
   temp_arena Temp = TempArena(0);
   string CLabel = CStrFromStr(Temp.Arena, Label);
   f32 CurrentVal = Abs(*Value);
   f32 Speed = PowF32(CurrentVal, 0.7f) / 100.0f;
   Speed = ClampBot(0.001f, Speed);
   char const *Format = ValueFormat;
   if (Format == 0) Format = "%.3f";
   b32 Result = Cast(b32)ImGui::DragFloat(CLabel.Data, Value, Speed, MinValue,
                                          MaxValue, Format, ImGuiSliderFlags_NoRoundToFormat);
   EndTemp(Temp);
   
   return Result;
}

internal b32
UI_DragFloatF(f32 *Value, f32 MinValue, f32 MaxValue, char const *ValueFormat, char const *Format, ...)
{
   temp_arena Temp = TempArena(0);
   va_list Args;
   va_start(Args, Format);
   string Label = StrFV(Temp.Arena, Format, Args);
   b32 Result = UI_DragFloat(Value, MinValue, MaxValue, ValueFormat, Label);
   va_end(Args);
   EndTemp(Temp);
   
   return Result;
}

internal b32
UI_DragFloat2(f32 Values[2], f32 MinValue, f32 MaxValue, char const *ValueFormat, string Label)
{
   temp_arena Temp = TempArena(0);
   string CLabel = CStrFromStr(Temp.Arena, Label);
   f32 CurrentVal = (Abs(Values[0]) + Abs(Values[1])) / 2;
   f32 Speed = PowF32(CurrentVal, 0.7f) / 100.0f;
   Speed = ClampBot(0.001f, Speed);
   char const *Format = ValueFormat;
   if (Format == 0) Format = "%.3f";
   b32 Result = Cast(b32)ImGui::DragFloat2(CLabel.Data, Values, Speed, MinValue,
                                           MaxValue, Format, ImGuiSliderFlags_NoRoundToFormat);
   EndTemp(Temp);
   
   return Result;
}

internal b32
UI_DragFloat2F(f32 Values[2], f32 MinValue, f32 MaxValue, char const *ValueFormat, char const *Format, ...)
{
   temp_arena Temp = TempArena(0);
   va_list Args;
   va_start(Args, Format);
   string Label = StrFV(Temp.Arena, Format, Args);
   b32 Result = UI_DragFloat2(Values, MinValue, MaxValue, ValueFormat, Label);
   va_end(Args);
   EndTemp(Temp);
   
   return Result;
}

internal b32
UI_SliderFloat(f32 *Value, f32 MinValue, f32 MaxValue, string Label)
{
   temp_arena Temp = TempArena(0);
   string CLabel = CStrFromStr(Temp.Arena, Label);
   b32 Result = Cast(b32)ImGui::SliderFloat(CLabel.Data, Value, MinValue, MaxValue);
   EndTemp(Temp);
   
   return Result;
}

internal b32
UI_SliderFloatF(f32 *Value, f32 MinValue, f32 MaxValue, char const *Format, ...)
{
   temp_arena Temp = TempArena(0);
   va_list Args;
   va_start(Args, Format);
   string Label = StrFV(Temp.Arena, Format, Args);
   b32 Result = UI_SliderFloat(Value, MinValue, MaxValue, Label);
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

internal not_collapsed_b32
UI_BeginWindow(b32 *IsOpen, string Label)
{
   temp_arena Temp = TempArena(0);
   string CLabel = CStrFromStr(Temp.Arena, Label);
   bool IsOpen_ = false;
   if (IsOpen) IsOpen_ = Cast(bool)(*IsOpen);
   b32 Result = Cast(b32)ImGui::Begin(CLabel.Data, (IsOpen ? &IsOpen_ : 0), ImGuiWindowFlags_AlwaysAutoResize);
   if (IsOpen) *IsOpen = Cast(b32)IsOpen_;
   EndTemp(Temp);
   
   return Result;
}

internal not_collapsed_b32
UI_BeginWindowF(b32 *IsOpen, char const *Format, ...)
{
   temp_arena Temp = TempArena(0);
   va_list Args;
   va_start(Args, Format);
   string Label = StrFV(Temp.Arena, Format, Args);
   b32 Result = UI_BeginWindow(IsOpen, Label);
   va_end(Args);
   EndTemp(Temp);
   
   return Result;
}

internal void UI_EndWindow(void) { ImGui::End(); }

internal not_collapsed_b32
UI_CollapsingHeader(string Label)
{
   temp_arena Temp = TempArena(0);
   string CLabel = CStrFromStr(Temp.Arena, Label);
   b32 Result = !(Cast(b32)ImGui::CollapsingHeader(CLabel.Data));
   EndTemp(Temp);
   
   return Result;
}

internal not_collapsed_b32
UI_CollapsingHeaderF(char const *Format, ...)
{
   temp_arena Temp = TempArena(0);
   va_list Args;
   va_start(Args, Format);
   string Label = StrFV(Temp.Arena, Format, Args);
   b32 Result = UI_CollapsingHeader(Label);
   va_end(Args);
   EndTemp(Temp);
   
   return Result;
}

internal clicked_b32
UI_SelectableItem(b32 Selected, string Label)
{
   temp_arena Temp = TempArena(0);
   string CLabel = CStrFromStr(Temp.Arena, Label);
   b32 Result = Cast(b32)ImGui::Selectable(CLabel.Data, Cast(bool)Selected);
   EndTemp(Temp);
   
   return Result;
}

internal clicked_b32
UI_SelectableItemF(b32 Selected, char const *Format, ...)
{
   temp_arena Temp = TempArena(0);
   va_list Args;
   va_start(Args, Format);
   string Label = StrFV(Temp.Arena, Format, Args);
   b32 Result = UI_SelectableItem(Selected, Label);
   va_end(Args);
   EndTemp(Temp);
   
   return Result;
}

internal void
UI_OpenPopup(string Label)
{
   temp_arena Temp = TempArena(0);
   string CLabel = CStrFromStr(Temp.Arena, Label);
   ImGui::OpenPopup(CLabel.Data);
   EndTemp(Temp);
}

internal open_b32
UI_BeginPopup(string Label)
{
   temp_arena Temp = TempArena(0);
   string CLabel = CStrFromStr(Temp.Arena, Label);
   b32 Result = Cast(b32)ImGui::BeginPopup(CLabel.Data);
   EndTemp(Temp);
   
   return Result;
}

internal void UI_EndPopup(void) { ImGui::EndPopup(); }

internal b32
UI_MenuItem(b32 *Selected, char const *Shortcut, string Label)
{
   temp_arena Temp = TempArena(0);
   string CLabel = CStrFromStr(Temp.Arena, Label);
   bool Selected_ = false;
   if (Selected) Selected_ = Cast(bool)(*Selected);
   b32 Result = Cast(b32)ImGui::MenuItem(CLabel.Data, Shortcut, &Selected_);
   if (Selected) *Selected = Cast(b32)Selected_;
   EndTemp(Temp);
   
   return Result;
}

internal b32
UI_MenuItemF(b32 *Selected, char const *Shortcut, char const *Format, ...)
{
   temp_arena Temp = TempArena(0);
   va_list Args;
   va_start(Args, Format);
   string Label = StrFV(Temp.Arena, Format, Args);
   b32 Result = UI_MenuItem(Selected, Shortcut, Label);
   EndTemp(Temp);
   
   return Result;
}

internal b32
UI_BeginMainMenuBar(void)
{
   b32 Result = Cast(b32)ImGui::BeginMainMenuBar();
   return Result;
}

internal void UI_EndMainMenuBar(void)   { ImGui::EndMainMenuBar(); }

internal b32
UI_BeginMenu(string Label)
{
   temp_arena Temp = TempArena(0);
   string CLabel = CStrFromStr(Temp.Arena, Label);
   b32 Result = Cast(b32)ImGui::BeginMenu(CLabel.Data);
   EndTemp(Temp);
   
   return Result;
}

internal b32
UI_BeginMenuF(char const *Format, ...)
{
   temp_arena Temp = TempArena(0);
   va_list Args;
   va_start(Args, Format);
   string Label = StrFV(Temp.Arena, Format, Args);
   b32 Result = UI_BeginMenu(Label);
   EndTemp(Temp);
   
   return Result;
}

internal void UI_EndMenu(void) { ImGui::EndMenu(); }

internal b32
UI_BeginCombo(string Preview, string Label)
{
   temp_arena Temp = TempArena(0);
   string CLabel = CStrFromStr(Temp.Arena, Label);
   string CPreview = CStrFromStr(Temp.Arena, Preview);
   b32 Result = Cast(b32)ImGui::BeginCombo(CLabel.Data, CPreview.Data);
   EndTemp(Temp);
   
   return Result;
}

internal b32
UI_BeginComboF(string Preview, char const *Format, ...)
{
   temp_arena Temp = TempArena(0);
   va_list Args;
   va_start(Args, Format);
   string Label = StrFV(Temp.Arena, Format, Args);
   b32 Result = UI_BeginCombo(Preview, Label);
   va_end(Args);
   EndTemp(Temp);
   
   return Result;
}

internal void UI_EndCombo(void) { ImGui::EndCombo(); }

internal b32
UI_BeginPopupModal(string Label)
{
   temp_arena Temp = TempArena(0);
   string CLabel = CStrFromStr(Temp.Arena, Label);
   b32 Result = Cast(b32)ImGui::BeginPopupModal(CLabel.Data, 0,
                                                ImGuiWindowFlags_NoTitleBar |
                                                ImGuiWindowFlags_AlwaysAutoResize);
   EndTemp(Temp);
   
   return Result;
}

internal void UI_CloseCurrentPopup(void) { ImGui::CloseCurrentPopup(); }