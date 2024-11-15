//- stb
#define STBI_NO_STDIO
#define STB_IMAGE_IMPLEMENTATION
#include "third_party/stb/stb_image.h"

//- imgui
#include "third_party/imgui/imgui.cpp"
#if BUILD_DEBUG
# include "third_party/imgui/imgui_demo.cpp"
#endif
#include "third_party/imgui/imgui_draw.cpp"
#include "third_party/imgui/imgui_tables.cpp"
#include "third_party/imgui/imgui_widgets.cpp"

//- editor imgui bindings
#include "base_ctx_crack.h"
#include "base_core.h"
#include "imgui_bindings.h"

#if OS_WINDOWS

#include "win32_imgui_bindings.h"
#include "third_party/imgui/imgui_impl_win32.cpp"
#include "third_party/imgui/imgui_impl_opengl3.cpp"

IMGUI_INIT_FUNC()
{
 ImGui::CreateContext();
 ImGui_ImplWin32_Init(Init->Window);
 ImGui_ImplOpenGL3_Init();
}

IMGUI_NEW_FRAME_FUNC()
{
 ImGui_ImplOpenGL3_NewFrame();
 ImGui_ImplWin32_NewFrame();
 ImGui::NewFrame();
}

IMGUI_RENDER_FUNC()
{
 ImGui::Render();
 ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

IMGUI_MAYBE_CAPTURE_INPUT_FUNC()
{
 imgui_maybe_capture_input_result Result = {};
 Result.CapturedInput = Cast(b32)ImGui_ImplWin32_WndProcHandler(Input->Window, Input->Msg, Input->wParam, Input->lParam);
 Result.ImGuiWantCaptureMouse = Cast(b32)ImGui::GetIO().WantCaptureMouse;
 
 return Result;
}

#elif OS_LINUX
# error not implemented
#else
# error unsupported OS
#endif