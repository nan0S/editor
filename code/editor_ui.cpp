#define UI_PushColorHandleVar  NameConcat(__PushColorHandle, __LINE__)

internal v2
V2FromImVec2(ImVec2 Vec2)
{
 v2 Result = V2(Vec2.x, Vec2.y);
 return Result;
}

internal ImVec2
ImVec2FromV2(v2 V)
{
 ImVec2 Result = ImVec2(V.X, V.Y);
 return Result;
}

internal b32
UI_IsItemHovered(void)
{
 b32 Result = (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled));
 return Result;
}

internal ImGuiMouseButton
UIMouseButtonToImGuiMouseButton(ui_mouse_button Button)
{
 ImGuiMouseButton Result = 0;
 switch (Button)
 {
  case UIMouseButton_Left: {Result = ImGuiMouseButton_Left;}break;
  case UIMouseButton_Right: {Result = ImGuiMouseButton_Right;}break;
  case UIMouseButton_Middle: {Result = ImGuiMouseButton_Middle;}break;
 }
 return Result;
}

internal b32
UI_IsMouseClicked(ui_mouse_button Button)
{
 ImGuiMouseButton ImButton = UIMouseButtonToImGuiMouseButton(Button);
 b32 Result = Cast(b32)ImGui::IsMouseClicked(ImButton);
 
 return Result;
}

internal b32
UI_IsWindowHovered(void)
{
 b32 Result = Cast(b32)ImGui::IsWindowHovered();
 return Result;
}

global v2 UI_GlobalNextItemSize;
global b32 UI_GlobalNextItemSizeSet;
internal void
UI_SetNextItemSize(v2 Size)
{
 UI_GlobalNextItemSizeSet = true;
 UI_GlobalNextItemSize = Size;
}

internal void
UI_SetNextItemPos(v2 Pos)
{
 ImVec2 ImPos(Pos.X, Pos.Y);
 ImGui::SetCursorPos(ImPos);
}

internal ImGuiCond
UICondToImGuiCond(ui_cond Cond)
{
 ImGuiCond Result = 0;
 switch (Cond)
 {
  case UICond_None: {Result=ImGuiCond_None;}break;
  case UICond_Always: {Result=ImGuiCond_Always;}break;
  case UICond_Once: {Result=ImGuiCond_Once;}break;
 }
 return Result;
}

internal void
UI_SetNextItemOpen(b32 Open, ui_cond Cond)
{
 ImGuiCond ImCond = UICondToImGuiCond(Cond);
 ImGui::SetNextItemOpen(Cast(bool)Open, ImCond);
}

internal ImVec2
UIPlacementToImGuiPivot(ui_placement Placement)
{
 ImVec2 Pivot = {};
 switch (Placement)
 {
  case UIPlacement_TopLeftCorner: {Pivot = ImVec2(0.0f, 0.0f);}break;
  case UIPlacement_TopRightCorner: {Pivot = ImVec2(1.0f, 0.0f);}break;
  case UIPlacement_BotLeftCorner: {Pivot = ImVec2(0.0f, 1.0f);}break;
  case UIPlacement_BotRightCorner: {Pivot = ImVec2(1.0f, 1.0f);}break;
  case UIPlacement_Center: {Pivot = ImVec2(0.5f, 0.5f);}break;
 }
 
 return Pivot;
}

internal void
UI_SetNextWindowPos(v2 P, ui_placement Placement)
{
 ImVec2 ImP = ImVec2FromV2(P);
 ImVec2 Pivot = UIPlacementToImGuiPivot(Placement);
 ImGui::SetNextWindowPos(ImP, ImGuiCond_Always, Pivot);
}

internal void
UI_SetNextWindowSizeConstraints(v2 MinSize, v2 MaxSize)
{
 ImVec2 ImMinSize = ImVec2FromV2(MinSize);
 ImVec2 ImMaxSize = ImVec2FromV2(MaxSize);
 ImGui::SetNextWindowSizeConstraints(ImMinSize, ImMaxSize);
}

internal void
UI_BringCurrentWindowToDisplayFront(void)
{
 ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
}

internal b32
UI_Combo(u32 *Enum, u32 EnumCount, string EnumNames[], string Label)
{
 temp_arena Temp = TempArena(0);
 string CLabel = CStrFromStr(Temp.Arena, Label);
 int Enum_ = *Enum;
 char **CEnumNames = PushArrayNonZero(Temp.Arena, EnumCount, char *);
 for (u32 EnumIndex = 0;
      EnumIndex < EnumCount;
      ++EnumIndex)
 {
  CEnumNames[EnumIndex] = CStrFromStr(Temp.Arena, EnumNames[EnumIndex]).Data;
 }
 b32 Result = Cast(b32)ImGui::Combo(CLabel.Data, &Enum_, CEnumNames, EnumCount);
 *Enum = Enum_;
 EndTemp(Temp);
 
 return Result;
}

internal b32
UI_ComboF(u32 *Enum, u32 EnumCount, string EnumNames[], char const *Format, ...)
{
 temp_arena Temp = TempArena(0);
 va_list  Args;
 va_start(Args, Format);
 string Label = StrFV(Temp.Arena, Format, Args);
 b32 Result = UI_Combo(Enum, EnumCount, EnumNames, Label);
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
UI_ColorPicker(v4 *Color, string Label)
{
 temp_arena Temp = TempArena(0);
 string CLabel = CStrFromStr(Temp.Arena, Label);
 b32 Result = Cast(b32)ImGui::ColorEdit4(CLabel.Data, Color->E);
 EndTemp(Temp);
 
 return Result;
}

internal b32
UI_ColorPickerF(v4 *Color, char const *Format, ...)
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
 ImVec2 Size = ImVec2(0, 0);
 if (UI_GlobalNextItemSizeSet)
 {
  Size = ImVec2(UI_GlobalNextItemSize.X, UI_GlobalNextItemSize.Y);
  UI_GlobalNextItemSizeSet = false;
 }
 b32 Result = Cast(b32)ImGui::Button(CLabel.Data, Size);
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
UI_AngleSlider(v2 *Rotation, string Label)
{
 temp_arena Temp = TempArena(0);
 string CLabel = CStrFromStr(Temp.Arena, Label);
 f32 RotationRad = Rotation2DToRadians(*Rotation);
 ImGui::SliderAngle(CLabel.Data, &RotationRad);
 *Rotation = Rotation2DFromRadians(RotationRad);
 EndTemp(Temp);
}

internal void
UI_AngleSliderF(v2 *Rotation, char const *Format, ...)
{
 temp_arena Temp = TempArena(0);
 va_list Args;
 va_start(Args, Format);
 string Label = StrFV(Temp.Arena, Format, Args);
 UI_AngleSlider(Rotation, Label);
 va_end(Args);
 EndTemp(Temp);
}

internal void
PushTightInputText(u32 InputWidthInChars)
{
 if (InputWidthInChars)
 {
  float FontSize = ImGui::GetFontSize();
  ImGui::PushItemWidth(FontSize * InputWidthInChars);
 }
}

internal void
PopTightInputText(u32 InputWidthInChars)
{
 if (InputWidthInChars)
 {
  ImGui::PopItemWidth();
 }
}

internal ui_input_result
UI_InputText(char *Buf, u64 BufSize, u32 InputWidthInChars, string Label)
{
 ui_input_result Result = {};
 temp_arena Temp = TempArena(0);
 string CLabel = CStrFromStr(Temp.Arena, Label);
 PushTightInputText(InputWidthInChars);
 Result.Changed = Cast(b32)ImGui::InputText(CLabel.Data, Buf, BufSize);
 PopTightInputText(InputWidthInChars);
 Result.Input = StrFromCStr(Buf);
 EndTemp(Temp);
 
 return Result;
}

internal ui_input_result
UI_InputTextF(char *Buf, u64 BufSize, u32 InputWidthInChars, char const *Format, ...)
{
 temp_arena Temp = TempArena(0);
 va_list Args;
 va_start(Args, Format);
 string Label = StrFV(Temp.Arena, Format, Args);
 ui_input_result Result = UI_InputText(Buf, BufSize, InputWidthInChars, Label);
 va_end(Args);
 EndTemp(Temp);
 
 return Result;
}

internal ui_input_result
UI_MultiLineInputText(char *Buf, u64 BufSize, u32 InputWidthInChars, string Label)
{
 ui_input_result Result = {};
 temp_arena Temp = TempArena(0);
 string CLabel = CStrFromStr(Temp.Arena, Label);
 PushTightInputText(InputWidthInChars);
 Result.Changed = Cast(b32)ImGui::InputTextMultiline(CLabel.Data, Buf, BufSize, ImVec2(0, 0), ImGuiInputTextFlags_AllowTabInput);
 PopTightInputText(InputWidthInChars);
 Result.Input = StrFromCStr(Buf);
 EndTemp(Temp);
 
 return Result;
}

internal ui_input_result
UI_MultiLineInputTextF(char *Buf, u64 BufSize, u32 InputWidthInChars, char const *Format, ...)
{
 temp_arena Temp = TempArena(0);
 va_list Args;
 va_start(Args, Format);
 string Label = StrFV(Temp.Arena, Format, Args);
 ui_input_result Result = UI_MultiLineInputText(Buf, BufSize, InputWidthInChars, Label);
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

internal void UI_PushId(u32 Id) { ImGui::PushID(Id); }
internal void UI_PopId(void) { ImGui::PopID(); }
internal void UI_PopLabel(void) { UI_PopId(); }
internal void UI_BeginDisabled(b32 Disabled) { ImGui::BeginDisabled(Cast(bool)Disabled); }
internal void UI_EndDisabled(void) { ImGui::EndDisabled(); }

internal ImVec4
V4ToImColor(v4 Color)
{
 ImVec4 Result = ImVec4(Color.R, Color.G, Color.B, Color.A);
 return Result;
}

internal ui_push_color_handle
UI_PushColor(ui_color_apply Apply, v4 Color)
{
 ui_push_color_handle Result = {};
 
 ImGuiCol ImCols[3] = {};
 v4 Colors[3] = {};
 StaticAssert(ArrayCount(Colors) == ArrayCount(ImCols), ArrayLengthsMatch);
 u32 ColCount = 0;
 switch (Apply)
 {
  case UI_Color_Text: {
   ImCols[0] = ImGuiCol_Text;
   ColCount = 1;
  }break;
  
  case UI_Color_Item: {
   ImCols[0] = ImGuiCol_Button;
   Colors[0] = Color;
   ImCols[1] = ImGuiCol_ButtonHovered;
   Colors[1] = BrightenColor(Color, 0.5f);
   ImCols[2] = ImGuiCol_ButtonActive;
   Colors[2] = Color;
   ColCount = 3;
  }break;
 }
 Assert(ColCount <= ArrayCount(ImCols));
 
 for (u32 ColIndex = 0;
      ColIndex < ColCount;
      ++ColIndex)
 {
  ImGui::PushStyleColor(ImCols[ColIndex], V4ToImColor(Colors[ColIndex]));
 }
 
 Result.PushColorCount = ColCount;
 
 return Result;
}

internal void
UI_PopColor(ui_push_color_handle Handle)
{
 ImGui::PopStyleColor(Handle.PushColorCount);
}

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
 // TODO(hbr): uncomment
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
SafeCastInt(i64 Value)
{
 AssertAlways(INT_MIN <= Value && Value <= INT_MAX);
 int Result = Cast(int)Value;
 return Result;
}

internal b32
UI_SliderInteger(i32 *Value, i32 MinValue, i32 MaxValue, string Label)
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
UI_SliderIntegerF(i32 *Value, i32 MinValue, i32 MaxValue, char const *Format, ...)
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
UI_Text(b32 Wrapped, string Text)
{
 temp_arena Temp = TempArena(0);
 string CText = CStrFromStr(Temp.Arena, Text);
 if (Wrapped)
 {
  ImGui::TextWrapped("%s", CText.Data);
 }
 else
 {
  ImGui::Text("%s", CText.Data);
 }
 EndTemp(Temp);
}

internal void
UI_TextF(b32 Wrapped, char const *Format, ...)
{
 va_list Args;
 va_start(Args, Format);
 temp_arena Temp = TempArena(0);
 string Text = StrFV(Temp.Arena, Format, Args);
 UI_Text(Wrapped, Text);
 EndTemp(Temp);
 va_end(Args);
}

StaticAssert(UIWindowFlag_Count == 4, SafetyCheckSoThatImplementationBelowChecksForAllUIWindowFlags);
internal ImGuiWindowFlags
UIWindowFlagsToImWindowFlags(ui_window_flags Flags)
{
 ImGuiWindowFlags ImFlags = 0;
 
 if (Flags & UIWindowFlag_AutoResize)         ImFlags |= ImGuiWindowFlags_AlwaysAutoResize;
 if (Flags & UIWindowFlag_NoTitleBar)         ImFlags |= ImGuiWindowFlags_NoDecoration;
 if (Flags & UIWindowFlag_NoFocusOnAppearing) ImFlags |= ImGuiWindowFlags_NoFocusOnAppearing;
 if (Flags & UIWindowFlag_NoMove)             ImFlags |= ImGuiWindowFlags_NoMove;
 
 return ImFlags;
}

internal not_collapsed_b32
UI_BeginWindow(b32 *IsOpen, ui_window_flags Flags, string Label)
{
 temp_arena Temp = TempArena(0);
 string CLabel = CStrFromStr(Temp.Arena, Label);
 bool IsOpen_ = false;
 if (IsOpen) IsOpen_ = Cast(bool)(*IsOpen);
 ImGuiWindowFlags ImFlags = UIWindowFlagsToImWindowFlags(Flags);
 b32 Result = Cast(b32)ImGui::Begin(CLabel.Data, (IsOpen ? &IsOpen_ : 0), ImFlags);
 if (IsOpen) *IsOpen = Cast(b32)IsOpen_;
 EndTemp(Temp);
 
 return Result;
}

internal not_collapsed_b32
UI_BeginWindowF(b32 *IsOpen, ui_window_flags Flags, char const *Format, ...)
{
 temp_arena Temp = TempArena(0);
 va_list Args;
 va_start(Args, Format);
 string Label = StrFV(Temp.Arena, Format, Args);
 b32 Result = UI_BeginWindow(IsOpen, Flags, Label);
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
UI_Tooltip(string Contents)
{
 temp_arena Temp = TempArena(0);
 string CContents = CStrFromStr(Temp.Arena, Contents);
 ImGui::SetTooltip(CContents.Data);
 EndTemp(Temp);
}

internal void
UI_TooltipF(char const *Format, ...)
{
 temp_arena Temp = TempArena(0);
 va_list Args;
 va_start(Args, Format);
 string Contents = StrFV(Temp.Arena, Format, Args);
 UI_Tooltip(Contents);
 va_end(Args);
 EndTemp(Temp);
}

internal b32
UI_Rect(u32 Id)
{
 b32 Clicked = false;
 UI_Id(Id)
 {
  Clicked = UI_Button(NilStr);
 }
 return Clicked;
}

internal void
UI_HorizontalSeparator(void)
{
 ImGui::Separator();
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
UI_BeginPopup(string Label, ui_window_flags Flags)
{
 temp_arena Temp = TempArena(0);
 string CLabel = CStrFromStr(Temp.Arena, Label);
 ImGuiWindowFlags ImFlags = UIWindowFlagsToImWindowFlags(Flags);
 b32 Result = Cast(b32)ImGui::BeginPopup(CLabel.Data, ImFlags);
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

internal b32 UI_BeginTree(string Label)
{
 temp_arena Temp = TempArena(0);
 string CLabel = CStrFromStr(Temp.Arena, Label);
 b32 Expanded = Cast(bool)ImGui::TreeNode(CLabel.Data);
 EndTemp(Temp);
 
 return Expanded;
}

internal b32
UI_BeginTreeF(char const *Format, ...)
{
 va_list Args;
 va_start(Args, Format);
 temp_arena Temp = TempArena(0);
 string Label = StrFV(Temp.Arena, Format, Args);
 b32 Result = UI_BeginTree(Label);
 EndTemp(Temp);
 va_end(Args);
 
 return Result;
}

internal void UI_EndTree(void) { ImGui::TreePop(); }

internal v2 UI_GetWindowSize(void)
{
 ImVec2 Size = ImGui::GetWindowSize();
 v2 Result = V2(Size.x, Size.y);
 return Result;
}

internal f32
UI_GetWindowWidth(void)
{
 f32 Result = UI_GetWindowSize().X;
 return Result;
}

internal f32
UI_GetWindowHeight(void)
{
 f32 Result = UI_GetWindowSize().Y;
 return Result;
}

internal rect2
UI_GetDrawableRegionBounds(void)
{
 ImVec2 ImCursor = ImGui::GetCursorPos();
 ImVec2 ImSize = ImGui::GetWindowContentRegionMax();
 
 v2 Cursor = V2FromImVec2(ImCursor);
 v2 Size = V2FromImVec2(ImSize);
 
 rect2 Result = Rect2(Cursor, Cursor + Size);
 
 return Result;
}