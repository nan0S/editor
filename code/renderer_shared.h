#ifndef RENDERER_SHARED_H
#define RENDERER_SHARED_H

#include "base/base_core.h"
#include "base/base_arena.h"
#include "base/base_string.h"
#include "base/base_os.h"
#include "base/base_thread_ctx.h"

#include "editor_profiler.h"
#include "editor_imgui_bindings.h"
#include "editor_platform.h"
#include "editor_renderer.h"

#include "base/base_core.cpp"
#include "base/base_arena.cpp"
#include "base/base_string.cpp"
#include "base/base_os.cpp"
#include "base/base_thread_ctx.cpp"

#include "editor_profiler.cpp"

platform_api Platform;

#endif //RENDERER_SHARED_H
