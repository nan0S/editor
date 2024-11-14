#ifndef WIN32_IMGUI_BINDINGS_H
#define WIN32_IMGUI_BINDINGS_H

struct imgui_init_data
{
 HWND Window;
};

struct imgui_maybe_capture_input_data
{
 HWND Window;
 UINT Msg;
 WPARAM wParam;
 LPARAM lParam;
};

#endif //WIN32_IMGUI_BINDINGS_H
