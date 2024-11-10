#pragma push_macro("Swap")
#pragma push_macro("Min")
#pragma push_macro("Max")
#undef Swap
#undef Min
#undef Max

#define IMGUI_DEFINE_MATH_OPERATORS
#include "third_party/imgui/imgui.h"
#include "third_party/imgui/imgui_internal.h"
#include "third_party/imgui/imgui_impl_opengl3.h"
#include "third_party/imgui/imgui_impl_win32.h"

#pragma pop_macro("Swap")
#pragma pop_macro("Min")
#pragma pop_macro("Max")