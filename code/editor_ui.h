#ifndef EDITOR_UI_H
#define EDITOR_UI_H

typedef b32 not_collapsed_b32;
typedef b32 clicked_b32;
typedef b32 changed_b32;
typedef b32 open_b32;

internal void UI_PushLabel(string Label);
internal void UI_PushLabelF(char const *Format, ...);
internal void UI_PushId(u64 Id);
internal void UI_PopId(void);
internal void UI_BeginDisabled(b32 Disabled) { ImGui::BeginDisabled(Cast(bool)Disabled); }
internal void UI_EndDisabled()   { ImGui::EndDisabled(); }

#define UI_Label(Label)        DeferBlock(UI_PushLabel(Label), UI_PopId())
#define UI_LabelF(Format, ...) DeferBlock(UI_PushLabelF(Label), UI_PopId())
#define UI_Id(Id)              DeferBlock(UI_PushId(Id), UI_PopId())
#define UI_Disabled(Disabled)  DeferBlock(UI_BeginDisabled(Disabled), UI_EndDisabled())

internal not_collapsed_b32 UI_BeginWindow(b32 *IsOpen, string Label);
internal not_collapsed_b32 UI_BeginWindowF(b32 *IsOpen, char const *Format, ...);
internal void              UI_EndWindow(void);

internal void              UI_OpenPopup(string Label);
internal open_b32          UI_BeginPopup(string Label);
internal open_b32          UI_BeginPopupModal(string Label);
internal void              UI_EndPopup(void);
internal void              UI_CloseCurrentPopup(void);

internal open_b32          UI_BeginMainMenuBar(void);
internal void              UI_EndMainMenuBar(void);
internal open_b32          UI_BeginMenu(string Label);
internal open_b32          UI_BeginMenuF(char const *Format, ...);
internal void              UI_EndMenu(void);

internal void UI_NewRow(void);
internal void UI_SameRow(void);

#define                    UI_Combo(InOutEnum, EnumCount, EnumNames, Label) UI_Combo_(Cast(u8 *)(InOutEnum), EnumCount, EnumNames, Label)
#define                    UI_ComboF(InOutEnum, EnumCount, EnumNames, Format, ...) UI_ComboF_(Cast(u8 *)(InOutEnum), EnumCount, EnumNames, Format, __VA_ARGS__)
internal void              UI_Checkbox(b32 *InOutEnabled, string Label);
internal void              UI_CheckboxF(b32 *InOutEnabled, char const *Format, ...);
internal clicked_b32       UI_Button(string Label);
internal clicked_b32       UI_ButtonF(char const *Format, ...);
internal changed_b32       UI_DragFloat(f32 *InOutValue, f32 MinValue, f32 MaxValue, char const *ValueFormat, string Label);
internal changed_b32       UI_DragFloatF(f32 *InOutValue, f32 MinValue, f32 MaxValue, char const *ValueFormat, char const *Format, ...);
internal changed_b32       UI_DragFloat2(f32 InOutValues[2], f32 MinValue, f32 MaxValue, char const *ValueFormat, string Label);
internal changed_b32       UI_DragFloat2F(f32 InOutValues[2], f32 MinValue, f32 MaxValue, char const *ValueFormat, char const *Format, ...);
internal changed_b32       UI_SliderFloat(f32 *InOutValue, f32 MinValue, f32 MaxValue, string Label);
internal changed_b32       UI_SliderFloatF(f32 *InOutValue, f32 MinValue, f32 MaxValue, char const *Format, ...);
#define                    UI_SliderInteger(Value, MinValue, MaxValue, Label) UI_SliderInteger_(Cast(s64 *)(Value), MinValue, MaxValue, Label)
#define                    UI_SliderIntegerF(Value, MinValue, MaxValue, Format, ...) UI_SliderIntegerF_(Cast(s64 *)(Value), MinValue, MaxValue, Format, __VA_ARGS__)
internal void              UI_AngleSlider(rotation_2d *InOutRotation, string Label);
internal void              UI_AngleSliderF(rotation_2d *InOutRotation, char const *Format, ...);
internal changed_b32       UI_ColorPicker(color *InOutColor, string Label);
internal changed_b32       UI_ColorPickerF(color *InOutColor, char const *Format, ...);
internal string            UI_InputText(char *Buf, u64 BufSize, string Label);
internal string            UI_InputTextF(char *Buf, u64 BufSize, char const *Format, ...);
internal void              UI_Text(string Text);
internal void              UI_TextF(char const *Format, ...);
internal void              UI_SeparatorText(string Text);
internal void              UI_SeparatorTextF(char const *Format, ...);
internal clicked_b32       UI_MenuItem(b32 *Selected, char const *Shortcut, string Label);
internal clicked_b32       UI_MenuItemF(b32 *Selected, char const *Shortcut, char const *Format, ...);
internal not_collapsed_b32 UI_CollapsingHeader(string Label);
internal not_collapsed_b32 UI_CollapsingHeaderF(char const *Format, ...);
internal clicked_b32       UI_SelectableItem(b32 Selected, string Label);
internal clicked_b32       UI_SelectableItemF(b32 Selected, char const *Format, ...);

#endif //EDITOR_UI_H