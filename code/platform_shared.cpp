#include "base/base_core.cpp"
#include "base/base_string.cpp"
#include "base/base_os.cpp"
#include "base/base_arena.cpp"
#include "base/base_thread_ctx.cpp"
#include "base/base_hot_reload.cpp"

#include "editor_work_queue.cpp"
#include "editor_profiler.cpp"

internal void
Platform_MakeWorkQueues(work_queue *LowPriorityQueue, work_queue *HighPriorityQueue)
{
 u32 ProcCount = OS_ProcCount();
 
 u32 LowPriorityThreadCount = ProcCount * 1/4;
 u32 HighPriorityThreadCount = ProcCount * 3/4;
 LowPriorityThreadCount = ClampBot(LowPriorityThreadCount, 1);
 HighPriorityThreadCount = ClampBot(HighPriorityThreadCount, 1);
 
 WorkQueueInit(LowPriorityQueue, LowPriorityThreadCount);
 WorkQueueInit(HighPriorityQueue, HighPriorityThreadCount);
}

internal editor_memory
Platform_MakeEditorMemory(arena *PermamentArena, renderer_memory *RendererMemory,
                          work_queue *LowPriorityQueue, work_queue *HighPriorityQueue,
                          platform_api PlatformAPI, profiler *Profiler)
{
 editor_memory EditorMemory = {};
 EditorMemory.PermamentArena = PermamentArena;
 EditorMemory.MaxTextureCount = RendererMemory->Limits.MaxTextureCount;
 EditorMemory.RendererQueue = &RendererMemory->RendererQueue;
 EditorMemory.LowPriorityQueue = LowPriorityQueue;
 EditorMemory.HighPriorityQueue = HighPriorityQueue;
 EditorMemory.PlatformAPI = PlatformAPI;
 EditorMemory.Profiler = Profiler;
 
 return EditorMemory;
}

internal renderer_memory
Platform_MakeRendererMemory(arena *PermamentArena, profiler *Profiler)
{
 renderer_memory RendererMemory = {};
 RendererMemory.PlatformAPI = Platform;
 
 platform_renderer_limits *Limits = &RendererMemory.Limits;
 // TODO(hbr): Revise those limits
 Limits->MaxTextureCount = 256;
 Limits->MaxBufferCount = 1024;
 
 renderer_transfer_queue *Queue = &RendererMemory.RendererQueue;
 Queue->TransferMemorySize = Megabytes(100);
 // TODO(hbr): use this value to test when memory renderer queue doesnt have space for an image
 //Queue->TransferMemorySize = 7680000 + 667152 - 1;
 Queue->TransferMemory = PushArrayNonZero(PermamentArena, Queue->TransferMemorySize, char);
 
 // TODO(hbr): Tweak these parameters
 RendererMemory.MaxLineCount = 1024;
 RendererMemory.LineBuffer = PushArrayNonZero(PermamentArena, RendererMemory.MaxLineCount, render_line);
 
 // TODO(hbr): Tweak these parameters
 RendererMemory.MaxCircleCount = 4096;
 RendererMemory.CircleBuffer = PushArrayNonZero(PermamentArena, RendererMemory.MaxCircleCount, render_circle);
 
 // TODO(hbr): Tweak these parameters
 RendererMemory.MaxImageCount = Limits->MaxTextureCount;
 RendererMemory.ImageBuffer = PushArrayNonZero(PermamentArena, RendererMemory.MaxImageCount, render_image);
 
 // TODO(hbr): Tweak these parameters
 RendererMemory.MaxVertexCount = 8 * 1024;
 RendererMemory.VertexBuffer = PushArrayNonZero(PermamentArena, RendererMemory.MaxVertexCount, render_vertex);
 
 RendererMemory.Profiler = Profiler;
 
 return RendererMemory;
}

internal platform_clock
Platform_MakeClock()
{
 platform_clock Clock = {};
 Clock.LastTSC = OS_ReadCPUTimer();
 Clock.CPU_TimerFreq = OS_CPUTimerFreq();
 
 return Clock;
}

internal f32
Platform_ClockFrame(platform_clock *Clock)
{
 u64 NowTSC = OS_ReadCPUTimer();
 f32 dtForFrame = Cast(f32)(NowTSC - Clock->LastTSC) / Clock->CPU_TimerFreq;
 Clock->LastTSC = NowTSC;
 
 return dtForFrame;
}

IMGUI_IS_ITEM_HOVERED(ImGuiIsItemHovered){return ImGui::IsItemHovered(flags);}
IMGUI_IS_MOUSE_CLICKED(ImGuiIsMouseClicked){return ImGui::IsMouseClicked(button);}
IMGUI_IS_WINDOW_HOVERED(ImGuiIsWindowHovered){return ImGui::IsWindowHovered(0);}
IMGUI_SET_CURSOR_POS(ImGuiSetCursorPos){ImGui::SetCursorPos(local_pos);}
IMGUI_SET_NEXT_ITEM_OPEN(ImGuiSetNextItemOpen){ImGui::SetNextItemOpen(is_open, cond);}
IMGUI_SET_NEXT_WINDOW_SIZE_CONSRAINTS(ImGuiSetNextWindowSizeConstraints){ImGui::SetNextWindowSizeConstraints(min_size, max_size);}
IMGUI_BRING_WINDOW_TO_DISPLAY_FRONT(ImGuiBringWindowToDisplayFront){ImGui::BringWindowToDisplayFront(window);}
IMGUI_GET_CURRENT_WINDOW(ImGuiGetCurrentWindow){return ImGui::GetCurrentWindow();}
IMGUI_COMBO(ImGuiCombo){return ImGui::Combo(label, current_item, items, items_count);}
IMGUI_CHECKBOX(ImGuiCheckbox){return ImGui::Checkbox(label, v);}
IMGUI_COLOR_EDIT4(ImGuiColorEdit4){return ImGui::ColorEdit4(label, col);}
IMGUI_BUTTON(ImGuiButton){return ImGui::Button(label, size);}
IMGUI_SLIDER_ANGLE(ImGuiSliderAngle){return ImGui::SliderAngle(label, v_rad);}
IMGUI_GET_FONT_SIZE(ImGuiGetFontSize){return ImGui::GetFontSize();}
IMGUI_PUSH_ITEM_WIDTH(ImGuiPushItemWidth){return ImGui::PushItemWidth(item_width);}
IMGUI_POP_ITEM_WIDTH(ImGuiPopItemWidth){return ImGui::PopItemWidth();}
IMGUI_INPUT_TEXT(ImGuiInputText){return ImGui::InputText(label, buf, buf_size);}
IMGUI_INPUT_TEXT_MULTILINE(ImGuiInputTextMultiline){return ImGui::InputTextMultiline(label, buf, buf_size, size, flags);}
IMGUI_PUSH_ID__STR(ImGuiPushID_Str){return ImGui::PushID(str_id);}
IMGUI_PUSH_ID__INT(ImGuiPushID_Int){return ImGui::PushID(int_id);}
IMGUI_POP_ID(ImGuiPopID){return ImGui::PopID();}
IMGUI_BEGIN_DISABLED(ImGuiBeginDisabled){return ImGui::BeginDisabled();}
IMGUI_END_DISABLED(ImGuiEndDisabled){return ImGui::EndDisabled();}
IMGUI_PUSH_STYLE_COLOR(ImGuiPushStyleColor){return ImGui::PushStyleColor(idx, col);}
IMGUI_POP_STYLE_COLOR(ImGuiPopStyleColor){return ImGui::PopStyleColor(count);}
IMGUI_PUSH_STYLE_VAR(ImGuiPushStyleVar){return ImGui::PushStyleVar(idx, val);}
IMGUI_POP_STYLE_VAR(ImGuiPopStyleVar){return ImGui::PopStyleVar();}
IMGUI_DRAG_FLOAT(ImGuiDragFloat){return ImGui::DragFloat(label, v, v_speed, v_min, v_max, format, flags);}
IMGUI_DRAG_FLOAT2(ImGuiDragFloat2){return ImGui::DragFloat2(label, v, v_speed, v_min, v_max, format, flags);}
IMGUI_SLIDER_FLOAT(ImGuiSliderFloat){return ImGui::SliderFloat(label, v, v_min, v_max);}
IMGUI_NEW_LINE(ImGuiNewLine){ImGui::NewLine();}
IMGUI_SAME_LINE(ImGuiSameLine){ImGui::SameLine();}
IMGUI_SEPARATOR_TEXT(ImGuiSeparatorText){ImGui::SeparatorText(label);}
IMGUI_SLIDER_INT(ImGuiSliderInt){return ImGui::SliderInt(label, v, v_min, v_max);}
IMGUI_COLLAPSING_HEADER(ImGuiCollapsingHeader){return ImGui::CollapsingHeader(label);}
IMGUI_SELECTABLE(ImGuiSelectable){return ImGui::Selectable(label, selectable);}
IMGUI_OPEN_POPUP(ImGuiOpenPopup){ImGui::OpenPopup(str_id);}
IMGUI_MENU_ITEM(ImGuiMenuItem){return ImGui::MenuItem(label, shortcut, p_selected);}
IMGUI_BEGIN_MENU(ImGuiBeginMenu){return ImGui::BeginMenu(label);}
IMGUI_BEGIN_COMBO(ImGuiBeginCombo){return ImGui::BeginCombo(label, preview_value);}
IMGUI_SHOW_DEMO_WINDOW(ImGuiShowDemoWindow){ImGui::ShowDemoWindow();}
IMGUI_SET_NEXT_WINDOW_POS(ImGuiSetNextWindowPos){ImGui::SetNextWindowPos(pos, cond, pivot);}

#define ImGuiTextWrapped ImGui::TextWrapped
#define ImGuiText ImGui::Text
#define ImGuiBegin ImGui::Begin
#define ImGuiEnd ImGui::End
#define ImGuiSetTooltip ImGui::SetTooltip
#define ImGuiSeparator ImGui::Separator
#define ImGuiBeginPopup ImGui::BeginPopup
#define ImGuiEndPopup ImGui::EndPopup
#define ImGuiBeginMainMenuBar ImGui::BeginMainMenuBar
#define ImGuiEndMainMenuBar ImGui::EndMainMenuBar
#define ImGuiEndMenu ImGui::EndMenu
#define ImGuiEndCombo ImGui::EndCombo
#define ImGuiBeginPopupModal ImGui::BeginPopupModal
#define ImGuiCloseCurrentPopup ImGui::CloseCurrentPopup
#define ImGuiTreeNode ImGui::TreeNode
#define ImGuiTreePop ImGui::TreePop
#define ImGuiGetWindowSize ImGui::GetWindowSize
#define ImGuiGetCursorPos ImGui::GetCursorPos
#define ImGuiGetWindowContentRegionMax ImGui::GetWindowContentRegionMax

internal platform_api
Platform_MakePlatformAPI(platform_open_file_dialog OpenFileDialog,
                         imgui_init *ImGuiInit,
                         imgui_new_frame *ImGuiNewFrame,
                         imgui_render *ImGuiRender,
                         imgui_maybe_capture_input *ImGuiMaybeCaptureInput)
{
 imgui_bindings ImGuiBindings = {
  ImGuiInit,
  ImGuiNewFrame,
  ImGuiRender,
  ImGuiMaybeCaptureInput,
  
  ImGuiIsItemHovered,
  ImGuiIsMouseClicked,
  ImGuiIsWindowHovered,
  ImGuiSetCursorPos,
  ImGuiSetNextItemOpen,
  ImGuiSetNextWindowSizeConstraints,
  ImGuiBringWindowToDisplayFront,
  ImGuiGetCurrentWindow,
  ImGuiCombo,
  ImGuiCheckbox,
  ImGuiColorEdit4,
  ImGuiButton,
  ImGuiSliderAngle,
  ImGuiGetFontSize,
  ImGuiPushItemWidth,
  ImGuiPopItemWidth,
  ImGuiInputText,
  ImGuiInputTextMultiline,
  ImGuiPushID_Str,
  ImGuiPushID_Int,
  ImGuiPopID,
  ImGuiBeginDisabled,
  ImGuiEndDisabled,
  ImGuiPushStyleColor,
  ImGuiPopStyleColor,
  ImGuiPushStyleVar,
  ImGuiPopStyleVar,
  ImGuiDragFloat,
  ImGuiDragFloat2,
  ImGuiSliderFloat,
  ImGuiNewLine,
  ImGuiSameLine,
  ImGuiSeparatorText,
  ImGuiSliderInt,
  ImGuiTextWrapped,
  ImGuiText,
  ImGuiBegin,
  ImGuiEnd,
  ImGuiCollapsingHeader,
  ImGuiSelectable,
  ImGuiSetTooltip,
  ImGuiSeparator,
  ImGuiOpenPopup,
  ImGuiBeginPopup,
  ImGuiEndPopup,
  ImGuiMenuItem,
  ImGuiBeginMainMenuBar,
  ImGuiEndMainMenuBar,
  ImGuiBeginMenu,
  ImGuiEndMenu,
  ImGuiBeginCombo,
  ImGuiEndCombo,
  ImGuiBeginPopupModal,
  ImGuiCloseCurrentPopup,
  ImGuiTreeNode,
  ImGuiTreePop,
  ImGuiGetWindowSize,
  ImGuiGetCursorPos,
  ImGuiGetWindowContentRegionMax,
  ImGuiShowDemoWindow,
  ImGuiSetNextWindowPos,
  
 };
 
 platform_api API = {
  OS_Reserve,
  OS_Release,
  OS_Commit,
  ThreadCtxGetScratch,
  OpenFileDialog,
  OS_ReadEntireFile,
  WorkQueueAddEntry,
  WorkQueueCompleteAllWork,
  ImGuiBindings,
 };
 
 return API;
}
