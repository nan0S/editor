#ifndef EDITOR_UI_H
#define EDITOR_UI_H

internal void UI_PushLabel(string Label);
internal void UI_PushLabelF(char const *Format, ...);
internal void UI_PushId(u64 Id);
internal void UI_PopId(void);

#define UI_Label(Label)        DeferBlock(UI_PushLabel(Label), UI_PopId())
#define UI_LabelF(Format, ...) DeferBlock(UI_PushLabelF(Label), UI_PopId())
#define UI_Id(Id)              DeferBlock(UI_PushId(Id), UI_PopId())

internal void UI_NewRow(void);
internal void UI_SameRow(void);
internal void UI_SeparatorText(string Text);
internal void UI_SeparatorTextF(char const *Format, ...);

#define         UI_Combo(InOutEnum, EnumCount, EnumNames, Label) UI_Combo_(Cast(u8 *)(InOutEnum), EnumCount, EnumNames, Label)
#define         UI_ComboF(InOutEnum, EnumCount, EnumNames, Format, ...) UI_ComboF_(Cast(u8 *)(InOutEnum), EnumCount, EnumNames, Format, __VA_ARGS__)
internal void   UI_Checkbox(b32 *InOutEnabled, string Label);
internal void   UI_CheckboxF(b32 *InOutEnabled, char const *Format, ...);
internal b32    UI_Button(string Label);
internal b32    UI_ButtonF(char const *Format, ...);
internal b32    UI_DragFloat(f32 *InOutValue, f32 MinValue, f32 MaxValue, char const *ValueFormat, string Label);
internal b32    UI_DragFloatF(f32 *InOutValue, f32 MinValue, f32 MaxValue, char const *ValueFormat, char const *Format, ...);
internal b32    UI_DragFloat2(f32 InOutValues[2], f32 MinValue, f32 MaxValue, char const *ValueFormat, string Label);
internal b32    UI_DragFloat2F(f32 InOutValues[2], f32 MinValue, f32 MaxValue, char const *ValueFormat, char const *Format, ...);
#define         UI_SliderInteger(Value, MinValue, MaxValue, Label) UI_SliderInteger_(Cast(s64 *)(Value), MinValue, MaxValue, Label)
#define         UI_SliderIntegerF(Value, MinValue, MaxValue, Format, ...) UI_SliderIntegerF_(Cast(s64 *)(Value), MinValue, MaxValue, Format, __VA_ARGS__)
internal void   UI_AngleSlider(rotation_2d *InOutRotation, string Label);
internal void   UI_AngleSliderF(rotation_2d *InOutRotation, char const *Format, ...);
internal b32    UI_ColorPicker(color *InOutColor, string Label);
internal b32    UI_ColorPickerF(color *InOutColor, char const *Format, ...);
internal string UI_InputText(char *Buf, u64 BufSize, string Label);
internal string UI_InputTextF(char *Buf, u64 BufSize, char const *Format, ...);
internal void   UI_Text(string Text);
internal void   UI_TextF(char const *Format, ...);

#endif //EDITOR_UI_H