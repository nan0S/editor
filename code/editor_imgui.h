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
typedef IMGUI_NEW_FRAME(imgui_NewFrame);

#define IMGUI_RENDER(Name) void Name(void)
typedef IMGUI_RENDER(imgui_Render);

#define ImGuiNonVariadicFunctions \
X_Macro(IsItemHovered, IsItemHovered, bool, ImGuiHoveredFlags flags, flags) \
X_Macro(IsMouseClicked, IsMouseClicked, bool, ImGuiMouseButton button, button) \
X_Macro(IsMouseDoubleClicked, IsMouseDoubleClicked, bool, ImGuiMouseButton button, button) \
X_Macro(IsWindowHovered, IsWindowHovered, bool, void, Nothing) \
X_Macro(SetCursorPos, SetCursorPos, void, ImVec2 local_pos, local_pos) \
X_Macro(SetNextItemOpen, SetNextItemOpen, void, bool is_open Comma ImGuiCond cond, cond Comma is_open) \
X_Macro(SetNextWindowSizeConstraints, SetNextWindowSizeConstraints, void, ImVec2 min_size Comma ImVec2 max_size, min_size Comma max_size) \
X_Macro(BringWindowToDisplayFront, BringWindowToDisplayFront, void, ImGuiWindow *window, window) \
X_Macro(GetCurrentWindow, GetCurrentWindow, ImGuiWindow *, void, Nothing) \
X_Macro(Combo, Combo, bool, const char* label Comma int* current_item Comma const char* const items[] Comma int items_count, label Comma current_item Comma items Comma items_count) \
X_Macro(Checkbox, Checkbox, bool, const char* label Comma bool* v, label Comma v) \
X_Macro(ColorEdit4, ColorEdit4, bool, const char* label Comma float col[4], label Comma col) \
X_Macro(Button, Button, bool, const char* label Comma ImVec2 size, label Comma size) \
X_Macro(SliderAngle, SliderAngle, bool, const char* label Comma float* v_rad, label Comma v_rad) \
X_Macro(GetFontSize, GetFontSize, float, void, Nothing) \
X_Macro(PushItemWidth, PushItemWidth, void, float item_width, item_width) \
X_Macro(PopItemWidth, PopItemWidth, void, void, Nothing) \
X_Macro(InputText, InputText, bool, const char* label Comma char* buf Comma size_t buf_size, label Comma buf Comma buf_size) \
X_Macro(InputTextMultiline, InputTextMultiline, bool, const char* label Comma char* buf Comma size_t buf_size Comma ImVec2 size Comma ImGuiInputTextFlags flags, label Comma buf Comma buf_size Comma size Comma flags) \
X_Macro(PopID, PopID, void, void, Nothing) \
X_Macro(BeginDisabled, BeginDisabled, void, bool disabled, disabled) \
X_Macro(EndDisabled, EndDisabled, void, void, Nothing) \
X_Macro(PushStyleColor, PushStyleColor, void, ImGuiCol idx Comma ImVec4 col, idx Comma col) \
X_Macro(PopStyleColor, PopStyleColor, void, int count, count) \
X_Macro(PushStyleVar, PushStyleVar, void, ImGuiStyleVar idx Comma float val, idx Comma val) \
X_Macro(PopStyleVar, PopStyleVar, void, void, Nothing) \
X_Macro(DragFloat, DragFloat, bool, const char* label Comma float* v Comma float v_speed Comma float v_min Comma float v_max Comma const char* format Comma ImGuiSliderFlags flags, label Comma v Comma v_speed Comma v_min Comma v_max Comma format Comma flags) \
X_Macro(DragFloat2, DragFloat2, bool, const char* label Comma float v[2] Comma float v_speed Comma float v_min Comma float v_max Comma const char* format Comma ImGuiSliderFlags flags, label Comma v Comma v_speed Comma v_min Comma v_max Comma format Comma flags) \
X_Macro(SliderFloat, SliderFloat, bool, const char* label Comma float* v Comma float v_min Comma float v_max, label Comma v Comma v_min Comma v_max) \
X_Macro(NewLine, NewLine, void, void, Nothing) \
X_Macro(SameLine, SameLine, void, void, Nothing) \
X_Macro(SeparatorText, SeparatorText, void, const char* label, label) \
X_Macro(SliderInt, SliderInt, bool, const char* label Comma int* v Comma int v_min Comma int v_max, label Comma v Comma v_min Comma v_max) \
X_Macro(Begin, Begin, bool, const char* name Comma bool* p_open Comma ImGuiWindowFlags flags, name Comma p_open Comma flags) \
X_Macro(End, End, void, void, Nothing) \
X_Macro(CollapsingHeader, CollapsingHeader, bool, const char* label, label) \
X_Macro(Selectable, Selectable, bool, const char* label Comma bool selectable, label Comma selectable) \
X_Macro(Separator, Separator, void, void, Nothing) \
X_Macro(OpenPopup, OpenPopup, void, const char* str_id, str_id) \
X_Macro(BeginPopup, BeginPopup, bool, const char* str_id Comma ImGuiWindowFlags flags, str_id Comma flags) \
X_Macro(EndPopup, EndPopup, void, void, Nothing) \
X_Macro(MenuItem, MenuItem, bool, const char* label Comma const char* shortcut Comma bool* p_selected, label Comma shortcut Comma p_selected) \
X_Macro(BeginMainMenuBar, BeginMainMenuBar, bool, void, Nothing) \
X_Macro(EndMainMenuBar, EndMainMenuBar, void, void, Nothing) \
X_Macro(BeginMenu, BeginMenu, bool, const char* label, label) \
X_Macro(EndMenu, EndMenu, void, void, Nothing) \
X_Macro(BeginCombo, BeginCombo, bool, const char* label Comma const char* preview_value, label Comma preview_value) \
X_Macro(EndCombo, EndCombo, void, void, Nothing) \
X_Macro(BeginPopupModal, BeginPopupModal, bool, const char* name Comma bool* p_open Comma ImGuiWindowFlags flags, name Comma p_open Comma flags) \
X_Macro(CloseCurrentPopup, CloseCurrentPopup, void, void, Nothing) \
X_Macro(TreeNode, TreeNode, bool, const char* label, label) \
X_Macro(TreePop, TreePop, void, void, Nothing) \
X_Macro(GetWindowSize, GetWindowSize, ImVec2, void, Nothing) \
X_Macro(GetCursorPos, GetCursorPos, ImVec2, void, Nothing) \
X_Macro(GetWindowContentRegionMax, GetWindowContentRegionMax, ImVec2, void, Nothing) \
X_Macro(GetContentRegionAvail, GetContentRegionAvail, ImVec2, void, Nothing) \
X_Macro(ShowDemoWindow, ShowDemoWindow, void, void, Nothing) \
X_Macro(SetNextWindowPos, SetNextWindowPos, void, ImVec2 pos Comma ImGuiCond cond Comma ImVec2 pivot, pos Comma cond Comma pivot) \
X_Macro(PushID_Str, PushID, void, const char* str_id, str_id) \
X_Macro(PushID_Int, PushID, void, int int_id, int_id) \
X_Macro(BeginTabBar, BeginTabBar, bool, const char* str_id, str_id) \
X_Macro(EndTabBar, EndTabBar, void, void, Nothing) \
X_Macro(BeginTabItem, BeginTabItem, bool, char const *label, label) \
X_Macro(EndTabItem, EndTabItem, void, void, Nothing) \
X_Macro(BeginTable, BeginTable, bool, const char* str_id Comma int column Comma ImGuiTableFlags flags, str_id Comma column Comma flags) \
X_Macro(EndTable, EndTable, void, Nothing, Nothing) \
X_Macro(TableNextRow, TableNextRow, void, Nothing, Nothing) \
X_Macro(TableNextColumn, TableNextColumn, bool, Nothing, Nothing) \
X_Macro(TableSetColumnIndex, TableSetColumnIndex, bool, int column_n, column_n) \

#define ImGuiVariadicFunctions \
X_Macro(Text, Text, void, const char* fmt Comma ..., fmt) \
X_Macro(SetTooltip, SetTooltip, void, const char* fmt Comma ..., fmt) \
X_Macro(BulletText, BulletText, void, const char* fmt Comma ..., fmt) \
X_Macro(TextWrapped, TextWrapped, void, const char* fmt Comma ..., fmt) \

#define ImGuiFunctions \
ImGuiNonVariadicFunctions \
ImGuiVariadicFunctions \

//- define function types
#define X_Macro(Name, ImGuiName, ReturnType, Args, ArgNames) typedef ReturnType imgui_##Name(Args);
ImGuiFunctions
#undef X_Macro

struct imgui_bindings
{
#define X_Macro(Name, ImGuiName, ReturnType, Args, ArgNames) imgui_##Name *Name;
 ImGuiFunctions
#undef X_Macro
};

#endif //EDITOR_IMGUI_BINDINGS_H