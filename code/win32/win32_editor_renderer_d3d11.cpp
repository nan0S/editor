#include <windows.h>

#include "base/base_ctx_crack.h"
#include "base/base_core.h"
#include "base/base_string.h"

#include "editor_memory.h"
#include "editor_imgui_bindings.h"
#include "editor_platform.h"
#include "editor_renderer.h"

#include "win32/win32_editor_renderer.h"

DLL_EXPORT
WIN32_RENDERER_INIT(Win32RendererInit)
{
 renderer *Renderer = 0;
 return Renderer;
}

DLL_EXPORT
RENDERER_BEGIN_FRAME(Win32RendererBeginFrame)
{
 render_frame *Frame = 0;
 return Frame;
}

DLL_EXPORT
RENDERER_END_FRAME(Win32RendererEndFrame)
{
 
}