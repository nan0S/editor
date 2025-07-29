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

internal void ShowDemoWindowStub(void){}

// NOTE(hbr): Don't define it in release because of high linking time
#if BUILD_DEBUG
# define ImGuiShowDemoWindow_X_Macro X_Macro(ShowDemoWindow, ImGui::ShowDemoWindow, void, void, Nothing)
#else
# define ImGuiShowDemoWindow_X_Macro X_Macro(ShowDemoWindow, ShowDemoWindowStub, void, void, Nothing)
#endif

#define ImGuiNonVariadicFunctions \
X_Macro(IsItemHovered, ImGui::IsItemHovered, bool, ImGuiHoveredFlags flags, flags) \
X_Macro(IsMouseClicked, ImGui::IsMouseClicked, bool, ImGuiMouseButton button, button) \
X_Macro(IsMouseDoubleClicked, ImGui::IsMouseDoubleClicked, bool, ImGuiMouseButton button, button) \
X_Macro(IsWindowHovered, ImGui::IsWindowHovered, bool, void, Nothing) \
X_Macro(SetCursorPos, ImGui::SetCursorPos, void, ImVec2 local_pos, local_pos) \
X_Macro(SetNextItemOpen, ImGui::SetNextItemOpen, void, bool is_open Comma ImGuiCond cond, cond Comma is_open) \
X_Macro(SetNextWindowSizeConstraints, ImGui::SetNextWindowSizeConstraints, void, ImVec2 min_size Comma ImVec2 max_size, min_size Comma max_size) \
X_Macro(BringWindowToDisplayFront, ImGui::BringWindowToDisplayFront, void, ImGuiWindow *window, window) \
X_Macro(GetCurrentWindow, ImGui::GetCurrentWindow, ImGuiWindow *, void, Nothing) \
X_Macro(Combo, ImGui::Combo, bool, const char* label Comma int* current_item Comma const char* const items[] Comma int items_count, label Comma current_item Comma items Comma items_count) \
X_Macro(Checkbox, ImGui::Checkbox, bool, const char* label Comma bool* v, label Comma v) \
X_Macro(ColorEdit4, ImGui::ColorEdit4, bool, const char* label Comma float col[4], label Comma col) \
X_Macro(Button, ImGui::Button, bool, const char* label Comma ImVec2 size, label Comma size) \
X_Macro(SliderAngle, ImGui::SliderAngle, bool, const char* label Comma float* v_rad, label Comma v_rad) \
X_Macro(GetFontSize, ImGui::GetFontSize, float, void, Nothing) \
X_Macro(PushItemWidth, ImGui::PushItemWidth, void, float item_width, item_width) \
X_Macro(PopItemWidth, ImGui::PopItemWidth, void, void, Nothing) \
X_Macro(InputText, ImGui::InputText, bool, const char* label Comma char* buf Comma size_t buf_size, label Comma buf Comma buf_size) \
X_Macro(InputTextMultiline, ImGui::InputTextMultiline, bool, const char* label Comma char* buf Comma size_t buf_size Comma ImVec2 size Comma ImGuiInputTextFlags flags, label Comma buf Comma buf_size Comma size Comma flags) \
X_Macro(PopID, ImGui::PopID, void, void, Nothing) \
X_Macro(BeginDisabled, ImGui::BeginDisabled, void, bool disabled, disabled) \
X_Macro(EndDisabled, ImGui::EndDisabled, void, void, Nothing) \
X_Macro(PushStyleColor, ImGui::PushStyleColor, void, ImGuiCol idx Comma ImVec4 col, idx Comma col) \
X_Macro(PopStyleColor, ImGui::PopStyleColor, void, int count, count) \
X_Macro(PushStyleVar, ImGui::PushStyleVar, void, ImGuiStyleVar idx Comma float val, idx Comma val) \
X_Macro(PopStyleVar, ImGui::PopStyleVar, void, void, Nothing) \
X_Macro(DragFloat, ImGui::DragFloat, bool, const char* label Comma float* v Comma float v_speed Comma float v_min Comma float v_max Comma const char* format Comma ImGuiSliderFlags flags, label Comma v Comma v_speed Comma v_min Comma v_max Comma format Comma flags) \
X_Macro(DragFloat2, ImGui::DragFloat2, bool, const char* label Comma float v[2] Comma float v_speed Comma float v_min Comma float v_max Comma const char* format Comma ImGuiSliderFlags flags, label Comma v Comma v_speed Comma v_min Comma v_max Comma format Comma flags) \
X_Macro(SliderFloat, ImGui::SliderFloat, bool, const char* label Comma float* v Comma float v_min Comma float v_max, label Comma v Comma v_min Comma v_max) \
X_Macro(NewLine, ImGui::NewLine, void, void, Nothing) \
X_Macro(SameLine, ImGui::SameLine, void, void, Nothing) \
X_Macro(SeparatorText, ImGui::SeparatorText, void, const char* label, label) \
X_Macro(SliderInt, ImGui::SliderInt, bool, const char* label Comma int* v Comma int v_min Comma int v_max, label Comma v Comma v_min Comma v_max) \
X_Macro(Begin, ImGui::Begin, bool, const char* name Comma bool* p_open Comma ImGuiWindowFlags flags, name Comma p_open Comma flags) \
X_Macro(End, ImGui::End, void, void, Nothing) \
X_Macro(CollapsingHeader, ImGui::CollapsingHeader, bool, const char* label, label) \
X_Macro(Selectable, ImGui::Selectable, bool, const char* label Comma bool selectable, label Comma selectable) \
X_Macro(Separator, ImGui::Separator, void, void, Nothing) \
X_Macro(OpenPopup, ImGui::OpenPopup, void, const char* str_id, str_id) \
X_Macro(BeginPopup, ImGui::BeginPopup, bool, const char* str_id Comma ImGuiWindowFlags flags, str_id Comma flags) \
X_Macro(EndPopup, ImGui::EndPopup, void, void, Nothing) \
X_Macro(MenuItem, ImGui::MenuItem, bool, const char* label Comma const char* shortcut Comma bool* p_selected, label Comma shortcut Comma p_selected) \
X_Macro(BeginMainMenuBar, ImGui::BeginMainMenuBar, bool, void, Nothing) \
X_Macro(EndMainMenuBar, ImGui::EndMainMenuBar, void, void, Nothing) \
X_Macro(BeginMenu, ImGui::BeginMenu, bool, const char* label, label) \
X_Macro(EndMenu, ImGui::EndMenu, void, void, Nothing) \
X_Macro(BeginCombo, ImGui::BeginCombo, bool, const char* label Comma const char* preview_value, label Comma preview_value) \
X_Macro(EndCombo, ImGui::EndCombo, void, void, Nothing) \
X_Macro(BeginPopupModal, ImGui::BeginPopupModal, bool, const char* name Comma bool* p_open Comma ImGuiWindowFlags flags, name Comma p_open Comma flags) \
X_Macro(CloseCurrentPopup, ImGui::CloseCurrentPopup, void, void, Nothing) \
X_Macro(TreeNode, ImGui::TreeNode, bool, const char* label, label) \
X_Macro(TreePop, ImGui::TreePop, void, void, Nothing) \
ImGuiShowDemoWindow_X_Macro \
X_Macro(GetWindowSize, ImGui::GetWindowSize, ImVec2, void, Nothing) \
X_Macro(GetCursorPos, ImGui::GetCursorPos, ImVec2, void, Nothing) \
X_Macro(GetWindowContentRegionMax, ImGui::GetWindowContentRegionMax, ImVec2, void, Nothing) \
X_Macro(GetContentRegionAvail, ImGui::GetContentRegionAvail, ImVec2, void, Nothing) \
X_Macro(SetNextWindowPos, ImGui::SetNextWindowPos, void, ImVec2 pos Comma ImGuiCond cond Comma ImVec2 pivot, pos Comma cond Comma pivot) \
X_Macro(PushID_Str, ImGui::PushID, void, const char* str_id, str_id) \
X_Macro(PushID_Int, ImGui::PushID, void, int int_id, int_id) \
X_Macro(BeginTabBar, ImGui::BeginTabBar, bool, const char* str_id, str_id) \
X_Macro(EndTabBar, ImGui::EndTabBar, void, void, Nothing) \
X_Macro(BeginTabItem, ImGui::BeginTabItem, bool, char const *label, label) \
X_Macro(EndTabItem, ImGui::EndTabItem, void, void, Nothing) \
X_Macro(BeginTable, ImGui::BeginTable, bool, const char* str_id Comma int column Comma ImGuiTableFlags flags, str_id Comma column Comma flags) \
X_Macro(EndTable, ImGui::EndTable, void, Nothing, Nothing) \
X_Macro(TableNextRow, ImGui::TableNextRow, void, Nothing, Nothing) \
X_Macro(TableNextColumn, ImGui::TableNextColumn, bool, Nothing, Nothing) \
X_Macro(TableSetColumnIndex, ImGui::TableSetColumnIndex, bool, int column_n, column_n) \
X_Macro(IsItemActivated, ImGui::IsItemActivated, bool, Nothing, Nothing) \
X_Macro(IsItemDeactivated, ImGui::IsItemDeactivated, bool, Nothing, Nothing) \

#define ImGuiVariadicFunctions \
X_Macro(Text, ImGui::Text, void, const char* fmt Comma ..., fmt) \
X_Macro(SetTooltip, ImGui::SetTooltip, void, const char* fmt Comma ..., fmt) \
X_Macro(BulletText, ImGui::BulletText, void, const char* fmt Comma ..., fmt) \
X_Macro(TextWrapped, ImGui::TextWrapped, void, const char* fmt Comma ..., fmt) \

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