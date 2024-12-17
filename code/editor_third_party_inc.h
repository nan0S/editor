#ifndef EDITOR_THIRD_PARTY_INC_H
#define EDITOR_THIRD_PARTY_INC_H

//#define STBI_NO_STDIO
#include "third_party/stb/stb_image.h"

#pragma push_macro("Max")
#pragma push_macro("Min")
#undef Max
#undef Min

#include "third_party/imgui/imgui.h"
#include "third_party/imgui/imgui_internal.h"

#pragma pop_macro("Max")
#pragma pop_macro("Min")

#if OS_WINDOWS
# include "third_party/imgui/imgui_impl_win32.h"
# include "third_party/imgui/imgui_impl_opengl3.h"
#elif OS_LINUX
# error not implemented
#else
# error unsupported OS
#endif

#endif //EDITOR_THIRD_PARTY_INC_H
