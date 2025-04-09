#ifndef EDITOR_IMGUI_BINDINGS_H
#define EDITOR_IMGUI_BINDINGS_H

#pragma push_macro("Max")
#pragma push_macro("Min")
#undef Max
#undef Min

#include "third_party/imgui/imgui.h"
#include "third_party/imgui/imgui_internal.h"

#pragma pop_macro("Max")
#pragma pop_macro("Min")

#define IMGUI_NEW_FRAME(Name) void Name(void)
typedef IMGUI_NEW_FRAME(imgui_new_frame);

#define IMGUI_RENDER(Name) void Name(void)
typedef IMGUI_RENDER(imgui_render);

#define IMGUI_IS_ITEM_HOVERED(Name) bool Name(ImGuiHoveredFlags flags)
typedef IMGUI_IS_ITEM_HOVERED(imgui_IsItemHovered);

#define IMGUI_IS_MOUSE_CLICKED(Name) bool Name(ImGuiMouseButton button)
typedef IMGUI_IS_MOUSE_CLICKED(imgui_IsMouseClicked);

#define IMGUI_IS_WINDOW_HOVERED(Name) bool Name(void)
typedef IMGUI_IS_WINDOW_HOVERED(imgui_IsWindowHovered);

#define IMGUI_SET_CURSOR_POS(Name) void Name(ImVec2 local_pos)
typedef IMGUI_SET_CURSOR_POS(imgui_SetCursorPos);

#define IMGUI_SET_NEXT_ITEM_OPEN(Name) void Name(bool is_open, ImGuiCond cond)
typedef IMGUI_SET_NEXT_ITEM_OPEN(imgui_SetNextItemOpen);

#define IMGUI_SET_NEXT_WINDOW_SIZE_CONSRAINTS(Name) void Name(ImVec2 min_size, ImVec2 max_size)
typedef IMGUI_SET_NEXT_WINDOW_SIZE_CONSRAINTS(imgui_SetNextWindowSizeConstraints);

#define IMGUI_BRING_WINDOW_TO_DISPLAY_FRONT(Name) void Name(ImGuiWindow *window)
typedef IMGUI_BRING_WINDOW_TO_DISPLAY_FRONT(imgui_BringWindowToDisplayFront);

#define IMGUI_GET_CURRENT_WINDOW(Name) ImGuiWindow *Name(void)
typedef IMGUI_GET_CURRENT_WINDOW(imgui_GetCurrentWindow);

#define IMGUI_COMBO(Name) bool Name(const char* label, int* current_item, const char* const items[], int items_count)
typedef IMGUI_COMBO(imgui_Combo);

#define IMGUI_CHECKBOX(Name) bool Name(const char* label, bool* v)
typedef IMGUI_CHECKBOX(imgui_Checkbox);

#define IMGUI_COLOR_EDIT4(Name) bool Name(const char* label, float col[4])
typedef IMGUI_COLOR_EDIT4(imgui_ColorEdit4);

#define IMGUI_BUTTON(Name) bool Name(const char* label, ImVec2 size)
typedef IMGUI_BUTTON(imgui_Button);

#define IMGUI_SLIDER_ANGLE(Name) bool Name(const char* label, float* v_rad)
typedef IMGUI_SLIDER_ANGLE(imgui_SliderAngle);

#define IMGUI_GET_FONT_SIZE(Name) float Name(void)
typedef IMGUI_GET_FONT_SIZE(imgui_GetFontSize);

#define IMGUI_PUSH_ITEM_WIDTH(Name) void Name(float item_width)
typedef IMGUI_PUSH_ITEM_WIDTH(imgui_PushItemWidth);

#define IMGUI_POP_ITEM_WIDTH(Name) void Name(void)
typedef IMGUI_POP_ITEM_WIDTH(imgui_PopItemWidth);

#define IMGUI_INPUT_TEXT(Name) bool Name(const char* label, char* buf, size_t buf_size)
typedef IMGUI_INPUT_TEXT(imgui_InputText);

#define IMGUI_INPUT_TEXT_MULTILINE(Name) bool Name(const char* label, char* buf, size_t buf_size, ImVec2 size, ImGuiInputTextFlags flags)
typedef IMGUI_INPUT_TEXT_MULTILINE(imgui_InputTextMultiline);

#define IMGUI_PUSH_ID__STR(Name) void Name(const char* str_id)
typedef IMGUI_PUSH_ID__STR(imgui_PushID_Str);

#define IMGUI_PUSH_ID__INT(Name) void Name(int int_id)
typedef IMGUI_PUSH_ID__INT(imgui_PushID_Int);

#define IMGUI_POP_ID(Name) void Name(void)
typedef IMGUI_POP_ID(imgui_PopID);

#define IMGUI_BEGIN_DISABLED(Name) void Name(bool disabled)
typedef IMGUI_BEGIN_DISABLED(imgui_BeginDisabled);

#define IMGUI_END_DISABLED(Name) void Name(void)
typedef IMGUI_END_DISABLED(imgui_EndDisabled);

#define IMGUI_PUSH_STYLE_COLOR(Name) void Name(ImGuiCol idx, ImVec4 col)
typedef IMGUI_PUSH_STYLE_COLOR(imgui_PushStyleColor);

#define IMGUI_POP_STYLE_COLOR(Name) void Name(int count)
typedef IMGUI_POP_STYLE_COLOR(imgui_PopStyleColor);

#define IMGUI_PUSH_STYLE_VAR(Name) void Name(ImGuiStyleVar idx, float val)
typedef IMGUI_PUSH_STYLE_VAR(imgui_PushStyleVar);

#define IMGUI_POP_STYLE_VAR(Name) void Name(void)
typedef IMGUI_POP_STYLE_VAR(imgui_PopStyleVar);

#define IMGUI_DRAG_FLOAT(Name) bool Name(const char* label, float* v, float v_speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
typedef IMGUI_DRAG_FLOAT(imgui_DragFloat);

#define IMGUI_DRAG_FLOAT2(Name) bool Name(const char* label, float v[2], float v_speed, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
typedef IMGUI_DRAG_FLOAT2(imgui_DragFloat2);

#define IMGUI_SLIDER_FLOAT(Name) bool Name(const char* label, float* v, float v_min, float v_max)
typedef IMGUI_SLIDER_FLOAT(imgui_SliderFloat);

#define IMGUI_NEW_LINE(Name) void Name(void)
typedef IMGUI_NEW_LINE(imgui_NewLine);

#define IMGUI_SAME_LINE(Name) void Name(void)
typedef IMGUI_SAME_LINE(imgui_SameLine);

#define IMGUI_SEPARATOR_TEXT(Name) void Name(const char* label)
typedef IMGUI_SEPARATOR_TEXT(imgui_SeparatorText);

#define IMGUI_SLIDER_INT(Name) bool Name(const char* label, int* v, int v_min, int v_max)
typedef IMGUI_SLIDER_INT(imgui_SliderInt);

#define IMGUI_TEXT_WRAPPED(Name) void Name(const char* fmt, ...)
typedef IMGUI_TEXT_WRAPPED(imgui_TextWrapped);

#define IMGUI_TEXT(Name) void Name(const char* fmt, ...)
typedef IMGUI_TEXT(imgui_Text);

#define IMGUI_BEGIN(Name) bool Name(const char* name, bool* p_open, ImGuiWindowFlags flags)
typedef IMGUI_BEGIN(imgui_Begin);

#define IMGUI_END(Name) void Name(void)
typedef IMGUI_END(imgui_End);

#define IMGUI_COLLAPSING_HEADER(Name) bool Name(const char* label)
typedef IMGUI_COLLAPSING_HEADER(imgui_CollapsingHeader);

#define IMGUI_SELECTABLE(Name) bool Name(const char* label, bool selectable)
typedef IMGUI_SELECTABLE(imgui_Selectable);

#define IMGUI_SET_TOOLTIP(Name) void Name(const char* fmt, ...)
typedef IMGUI_SET_TOOLTIP(imgui_SetTooltip);

#define IMGUI_SEPARATOR(Name) void Name(void)
typedef IMGUI_SEPARATOR(imgui_Separator);

#define IMGUI_OPEN_POPUP(Name) void Name(const char* str_id)
typedef IMGUI_OPEN_POPUP(imgui_OpenPopup);

#define IMGUI_BEGIN_POPUP(Name) bool Name(const char* str_id, ImGuiWindowFlags flags)
typedef IMGUI_BEGIN_POPUP(imgui_BeginPopup);

#define IMGUI_END_POPUP(Name) void Name(void)
typedef IMGUI_END_POPUP(imgui_EndPopup);

#define IMGUI_MENU_ITEM(Name) bool Name(const char* label, const char* shortcut, bool* p_selected)
typedef IMGUI_MENU_ITEM(imgui_MenuItem);

#define IMGUI_BEGIN_MAIN_MENU_BAR(Name) bool Name(void)
typedef IMGUI_BEGIN_MAIN_MENU_BAR(imgui_BeginMainMenuBar);

#define IMGUI_END_MAIN_MENU_BAR(Name) void Name(void)
typedef IMGUI_END_MAIN_MENU_BAR(imgui_EndMainMenuBar);

#define IMGUI_BEGIN_MENU(Name) bool Name(const char* label)
typedef IMGUI_BEGIN_MENU(imgui_BeginMenu);

#define IMGUI_END_MENU(Name) void Name(void)
typedef IMGUI_END_MENU(imgui_EndMenu);

#define IMGUI_BEGIN_COMBO(Name) bool Name(const char* label, const char* preview_value)
typedef IMGUI_BEGIN_COMBO(imgui_BeginCombo);

#define IMGUI_END_COMBO(Name) void Name(void)
typedef IMGUI_END_COMBO(imgui_EndCombo);

#define IMGUI_BEGIN_POPUP_MODAL(Name) bool Name(const char* name, bool* p_open, ImGuiWindowFlags flags)
typedef IMGUI_BEGIN_POPUP_MODAL(imgui_BeginPopupModal);

#define IMGUI_CLOSE_CURRENT_POPUP(Name) void Name(void)
typedef IMGUI_CLOSE_CURRENT_POPUP(imgui_CloseCurrentPopup);

#define IMGUI_TREE_NODE(Name) bool Name(const char* label)
typedef IMGUI_TREE_NODE(imgui_TreeNode);

#define IMGUI_TREE_POP(Name) void Name(void)
typedef IMGUI_TREE_POP(imgui_TreePop);

#define IMGUI_GET_WINDOW_SIZE(Name) ImVec2 Name(void)
typedef IMGUI_GET_WINDOW_SIZE(imgui_GetWindowSize);

#define IMGUI_GET_CURSOR_POS(Name) ImVec2 Name(void)
typedef IMGUI_GET_CURSOR_POS(imgui_GetCursorPos);

#define IMGUI_GET_WINDOW_CONTENT_REGION_MAX(Name) ImVec2 Name(void)
typedef IMGUI_GET_WINDOW_CONTENT_REGION_MAX(imgui_GetWindowContentRegionMax);

#define IMGUI_SHOW_DEMO_WINDOW(Name) void Name(void)
typedef IMGUI_SHOW_DEMO_WINDOW(imgui_ShowDemoWindow);

#define IMGUI_SET_NEXT_WINDOW_POS(Name) void Name(ImVec2 pos, ImGuiCond cond, ImVec2 pivot)
typedef IMGUI_SET_NEXT_WINDOW_POS(imgui_SetNextWindowPos);

struct imgui_bindings
{
#define ImGuiFunc(Name) imgui_##Name *Name
 
 ImGuiFunc(IsItemHovered);
 ImGuiFunc(IsMouseClicked);
 ImGuiFunc(IsWindowHovered);
 ImGuiFunc(SetCursorPos);
 ImGuiFunc(SetNextItemOpen);
 ImGuiFunc(SetNextWindowSizeConstraints);
 ImGuiFunc(BringWindowToDisplayFront);
 ImGuiFunc(GetCurrentWindow);
 ImGuiFunc(Combo);
 ImGuiFunc(Checkbox);
 ImGuiFunc(ColorEdit4);
 ImGuiFunc(Button);
 ImGuiFunc(SliderAngle);
 ImGuiFunc(GetFontSize);
 ImGuiFunc(PushItemWidth);
 ImGuiFunc(PopItemWidth);
 ImGuiFunc(InputText);
 ImGuiFunc(InputTextMultiline);
 ImGuiFunc(PushID_Str);
 ImGuiFunc(PushID_Int);
 ImGuiFunc(PopID);
 ImGuiFunc(BeginDisabled);
 ImGuiFunc(EndDisabled);
 ImGuiFunc(PushStyleColor);
 ImGuiFunc(PopStyleColor);
 ImGuiFunc(PushStyleVar);
 ImGuiFunc(PopStyleVar);
 ImGuiFunc(DragFloat);
 ImGuiFunc(DragFloat2);
 ImGuiFunc(SliderFloat);
 ImGuiFunc(NewLine);
 ImGuiFunc(SameLine);
 ImGuiFunc(SeparatorText);
 ImGuiFunc(SliderInt);
 ImGuiFunc(TextWrapped);
 ImGuiFunc(Text);
 ImGuiFunc(Begin);
 ImGuiFunc(End);
 ImGuiFunc(CollapsingHeader);
 ImGuiFunc(Selectable);
 ImGuiFunc(SetTooltip);
 ImGuiFunc(Separator);
 ImGuiFunc(OpenPopup);
 ImGuiFunc(BeginPopup);
 ImGuiFunc(EndPopup);
 ImGuiFunc(MenuItem);
 ImGuiFunc(BeginMainMenuBar);
 ImGuiFunc(EndMainMenuBar);
 ImGuiFunc(BeginMenu);
 ImGuiFunc(EndMenu);
 ImGuiFunc(BeginCombo);
 ImGuiFunc(EndCombo);
 ImGuiFunc(BeginPopupModal);
 ImGuiFunc(CloseCurrentPopup);
 ImGuiFunc(TreeNode);
 ImGuiFunc(TreePop);
 ImGuiFunc(GetWindowSize);
 ImGuiFunc(GetCursorPos);
 ImGuiFunc(GetWindowContentRegionMax);
 ImGuiFunc(ShowDemoWindow);
 ImGuiFunc(SetNextWindowPos);
 
#undef ImGuiFunc
};

#endif //EDITOR_IMGUI_BINDINGS_H