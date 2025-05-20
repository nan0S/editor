#ifndef EDITOR_UI_H
#define EDITOR_UI_H

typedef b32 not_collapsed_b32;
typedef b32 clicked_b32;
typedef b32 changed_b32;
typedef b32 open_b32;
typedef b32 expanded_b32;
typedef b32 selected_b32;

enum
{
 UIWindowFlag_AutoResize         = (1<<0),
 UIWindowFlag_NoTitleBar         = (1<<1),
 UIWindowFlag_NoFocusOnAppearing = (1<<2),
 UIWindowFlag_NoMove             = (1<<3),
 
 UIWindowFlag_Count              = 4,
};
typedef u32 ui_window_flags;

struct ui_input_result
{
 string Input;
 b32 Changed;
};

enum ui_color_apply
{
 UI_Color_Text,
 UI_Color_Item,
};

struct ui_push_color_handle
{
 u32 PushColorCount;
};

enum ui_cond
{
 UICond_None,
 UICond_Always,
 UICond_Once,
};

enum ui_mouse_button
{
 UIMouseButton_Left,
 UIMouseButton_Right,
 UIMouseButton_Middle,
};

enum ui_placement
{
 UIPlacement_TopLeftCorner,
 UIPlacement_TopRightCorner,
 UIPlacement_BotLeftCorner,
 UIPlacement_BotRightCorner,
 UIPlacement_Center,
};

internal void                 UI_Equip(imgui_bindings Bindings);

internal void                 UI_PushLabel(string Label);
internal void                 UI_PushLabelF(char const *Format, ...);
internal void                 UI_PushId(u32 Id);
internal void                 UI_PopLabel(void);
internal void                 UI_PopId(void);
internal void                 UI_BeginDisabled(b32 Disabled);
internal void                 UI_EndDisabled(void);
internal ui_push_color_handle UI_PushColor(ui_color_apply Apply, v4 Color);
internal void                 UI_PopColor(ui_push_color_handle Handle);
internal void                 UI_PushAlpha(f32 Alpha);
internal void                 UI_PopAlpha(void);

#define UI_Label(Label)           DeferBlock(UI_PushLabel(Label), UI_PopLabel())
#define UI_LabelF(...)            DeferBlock(UI_PushLabelF(__VA_ARGS__), UI_PopLabel())
#define UI_Id(Id)                 DeferBlock(UI_PushId(Id), UI_PopId())
#define UI_Disabled(Disabled)     DeferBlock(UI_BeginDisabled(Disabled), UI_EndDisabled())
#define UI_Colored(Apply, Color)  ui_push_color_handle NameConcat(UI_PushColorHandleVar, __LINE__) = UI_PushColor(Apply, Color); DeferBlock(NoOp, UI_PopColor(NameConcat(UI_PushColorHandleVar, __LINE__)))
#define UI_Alpha(Alpha)           DeferBlock(UI_PushAlpha(Alpha), UI_PopAlpha())

internal not_collapsed_b32 UI_BeginWindow(b32 *IsOpen, ui_window_flags Flags, string Label);
internal not_collapsed_b32 UI_BeginWindowF(b32 *IsOpen, ui_window_flags Flags, char const *Format, ...);
internal void              UI_EndWindow(void);

internal void              UI_OpenPopup(string Label);
internal open_b32          UI_BeginPopup(string Label, ui_window_flags Flags);
internal open_b32          UI_BeginPopupModal(string Label);
internal void              UI_EndPopup(void);
internal void              UI_CloseCurrentPopup(void);

internal open_b32          UI_BeginMainMenuBar(void);
internal void              UI_EndMainMenuBar(void);
internal open_b32          UI_BeginMenu(string Label);
internal open_b32          UI_BeginMenuF(char const *Format, ...);
internal void              UI_EndMenu(void);

internal expanded_b32      UI_BeginCombo(string Preview, string Label);
internal expanded_b32      UI_BeginComboF(string Preview, char const *Format, ...);
internal void              UI_EndCombo(void);

internal expanded_b32      UI_BeginTree(string Label);
internal expanded_b32      UI_BeginTreeF(char const *Format, ...);
internal void              UI_EndTree(void);

// TODO(hbr): what does the returned b32 mean? label it with typedef'ed b32 like the rest of the functions
internal b32               UI_BeginTabBar(string Label);
internal b32               UI_BeginTabBarF(char const *Format, ...);
internal void              UI_EndTabBar(void);

internal selected_b32      UI_BeginTabItem(string Label);
internal selected_b32      UI_BeginTabItemF(char const *Format, ...);
internal void              UI_EndTabItem(void);

internal void              UI_NewRow(void);
internal void              UI_SameRow(void);

internal b32               UI_IsItemHovered(void);
internal b32               UI_IsMouseClicked(ui_mouse_button Button);
internal b32               UI_IsWindowHovered(void);

internal void              UI_SetNextItemSize(v2 Size);
internal void              UI_SetNextItemPos(v2 TopLeftP); // top-left corner
internal void              UI_SetNextItemOpen(b32 Open, ui_cond Cond);
internal void              UI_SetNextWindowPos(v2 P, ui_placement Placement);
internal void              UI_SetNextWindowSizeConstraints(v2 MinSize, v2 MaxSize);

internal void              UI_BringCurrentWindowToDisplayFront(void);

internal v2                UI_GetWindowSize(void);
internal f32               UI_GetWindowWidth(void);
internal f32               UI_GetWindowHeight(void);
internal rect2             UI_GetDrawableRegionBounds(void); // window has title bar, scroll bar, get drawable part

internal changed_b32       UI_Combo(u32 *Enum, u32 EnumCount, string EnumNames[], string Label);
internal changed_b32       UI_ComboF(u32 *Enum, u32 EnumCount, string EnumNames[], char const *Format, ...);
internal changed_b32       UI_Checkbox(b32 *Enabled, string Label);
internal changed_b32       UI_CheckboxF(b32 *Enabled, char const *Format, ...);
internal clicked_b32       UI_Button(string Label);
internal clicked_b32       UI_ButtonF(char const *Format, ...);
internal changed_b32       UI_DragFloat(f32 *Value, f32 MinValue, f32 MaxValue, char const *ValueFormat, string Label);
internal changed_b32       UI_DragFloatF(f32 *Value, f32 MinValue, f32 MaxValue, char const *ValueFormat, char const *Format, ...);
internal changed_b32       UI_DragFloat2(f32 Values[2], f32 MinValue, f32 MaxValue, char const *ValueFormat, string Label);
internal changed_b32       UI_DragFloat2F(f32 Values[2], f32 MinValue, f32 MaxValue, char const *ValueFormat, char const *Format, ...);
internal changed_b32       UI_SliderInteger(i32 *Value, i32 MinValue, i32 MaxValue, string Label);
internal changed_b32       UI_SliderIntegerF(i32 *Value, i32 MinValue, i32 MaxValue, char const *Format, ...);
internal changed_b32       UI_SliderFloat(f32 *Value, f32 MinValue, f32 MaxValue, string Label);
internal changed_b32       UI_SliderFloatF(f32 *Value, f32 MinValue, f32 MaxValue, char const *Format, ...);
internal void              UI_AngleSlider(v2 *Rotation, string Label);
internal void              UI_AngleSliderF(v2 *Rotation, char const *Format, ...);
internal changed_b32       UI_ColorPicker(v4 *Color, string Label);
internal changed_b32       UI_ColorPickerF(v4 *Color, char const *Format, ...);
internal ui_input_result   UI_InputText2(arena *Arena, string Input, u32 InputWidthInChars, string Label);
internal ui_input_result   UI_InputText(char *Buf, u64 BufSize, u32 InputWidthInChars, string Label);
internal ui_input_result   UI_InputTextF(char *Buf, u64 BufSize, u32 InputWidthInChars, char const *Format, ...);
internal ui_input_result   UI_MultiLineInputText(char *Buf, u64 BufSize, u32 InputWidthInChars, string Label);
internal ui_input_result   UI_MultiLineInputTextF(char *Buf, u64 BufSize, u32 InputWidthInChars, char const *Format, ...);
internal void              UI_Text(b32 Wrapped, string Text);
internal void              UI_TextF(b32 Wrapped, char const *Format, ...);
internal void              UI_SeparatorText(string Text);
internal void              UI_SeparatorTextF(char const *Format, ...);
internal void              UI_BulletText(string Text);
internal void              UI_BulletTextF(char const *Format, ...);
internal clicked_b32       UI_MenuItem(b32 *Selected, char const *Shortcut, string Label);
internal clicked_b32       UI_MenuItemF(b32 *Selected, char const *Shortcut, char const *Format, ...);
internal not_collapsed_b32 UI_CollapsingHeader(string Label);
internal not_collapsed_b32 UI_CollapsingHeaderF(char const *Format, ...);
internal clicked_b32       UI_SelectableItem(b32 Selected, string Label);
internal clicked_b32       UI_SelectableItemF(b32 Selected, char const *Format, ...);
internal void              UI_Tooltip(string Contents);
internal void              UI_TooltipF(char const *Format, ...);
internal clicked_b32       UI_Rect(u32 Id);
internal void              UI_HorizontalSeparator(void);

internal void              UI_RenderDemoWindow(void);

#endif //EDITOR_UI_H