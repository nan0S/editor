#ifndef WIN32_EDITOR_IMGUI_BINDINGS_H
#define WIN32_EDITOR_IMGUI_BINDINGS_H

struct win32_imgui_init_data
{
 HWND Window;
};

struct win32_imgui_maybe_capture_input_data
{
 HWND Window;
 UINT Msg;
 WPARAM wParam;
 LPARAM lParam;
};

#endif //WIN32_EDITOR_IMGUI_BINDINGS_H
