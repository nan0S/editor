#include "base/base_core.h"

//- imgui
#pragma push_macro("Max")
#pragma push_macro("Min")
#undef Max
#undef Min

#include "third_party/imgui/imgui.cpp"
#if BUILD_DEBUG
# include "third_party/imgui/imgui_demo.cpp"
#endif
#include "third_party/imgui/imgui_draw.cpp"
#include "third_party/imgui/imgui_tables.cpp"
#include "third_party/imgui/imgui_widgets.cpp"

#if OS_WINDOWS

# include "win32/win32_editor_imgui_bindings.h"
# include "third_party/imgui/imgui_impl_win32.cpp"
# include "third_party/imgui/imgui_impl_opengl3.cpp"

#elif OS_LINUX
# error not implemented
#else
# error unsupported OS
#endif

#pragma pop_macro("Max")
#pragma pop_macro("Min")