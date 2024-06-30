#ifndef EDITOR_WINDOWS_H
#define EDITOR_WINDOWS_H

#pragma push_macro("function")
#undef function
#include <windows.h>
#include <windowsx.h>
#include <tlhelp32.h>
#include <Shlobj.h>
#include <intrin.h>
#pragma pop_macro("function")

typedef HANDLE file_handle;

#endif //EDITOR_WINDOWS_H
