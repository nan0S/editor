#ifndef EDITOR_THIRD_PARTY_INC_H
#define EDITOR_THIRD_PARTY_INC_H

#if OS_WINDOWS
# include "third_party/imgui/imgui_impl_win32.h"
# include "third_party/imgui/imgui_impl_opengl3.h"
#elif OS_LINUX
# error not implemented
#else
# error unsupported OS
#endif

#endif //EDITOR_THIRD_PARTY_INC_H
