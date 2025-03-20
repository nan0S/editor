#ifndef EDITOR_IMGUI_BINDINGS_H
#define EDITOR_IMGUI_BINDINGS_H

struct imgui_init_data;
#define IMGUI_INIT(Name) void Name(imgui_init_data *Init)
#define IMGUI_INIT_FUNC() IMGUI_INIT(ImGuiInit)
typedef IMGUI_INIT(imgui_init);

#define IMGUI_NEW_FRAME(Name) void Name(void)
#define IMGUI_NEW_FRAME_FUNC() IMGUI_NEW_FRAME(ImGuiNewFrame)
typedef IMGUI_NEW_FRAME(imgui_new_frame);

#define IMGUI_RENDER(Name) void Name(void)
#define IMGUI_RENDER_FUNC() IMGUI_RENDER(ImGuiRender)
typedef IMGUI_RENDER(imgui_render);

struct imgui_maybe_capture_input_data;
struct imgui_maybe_capture_input_result
{
 b32 CapturedInput;
 b32 ImGuiWantCaptureMouse;
 b32 ImGuiWantCaptureKeyboard;
};
#define IMGUI_MAYBE_CAPTURE_INPUT(Name) imgui_maybe_capture_input_result Name(imgui_maybe_capture_input_data *Input)
#define IMGUI_MAYBE_CAPTURE_INPUT_FUNC() IMGUI_MAYBE_CAPTURE_INPUT(ImGuiMaybeCaptureInput)
typedef IMGUI_MAYBE_CAPTURE_INPUT(imgui_maybe_capture_input);

struct imgui_bindings
{
 imgui_init *Init;
 imgui_new_frame *NewFrame;
 imgui_render *Render;
 imgui_maybe_capture_input *MaybeCaptureInput;
};

#endif //EDITOR_IMGUI_BINDINGS_H