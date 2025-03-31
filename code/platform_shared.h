#ifndef PLATFORM_SHARED_H
#define PLATFORM_SHARED_H

#include "base/base_core.h"
#include "base/base_string.h"
#include "base/base_os.h"
#include "base/base_arena.h"
#include "base/base_thread_ctx.h"
#include "base/base_hot_reload.h"

#include "editor_imgui.h"
#include "editor_platform.h"
#include "editor_work_queue.h"
#include "editor_renderer.h"
#include "editor_profiler.h"

#ifndef EDITOR_DLL
# error EDITOR_DLL with path to editor DLL code is not defined
#endif
#ifndef EDITOR_RENDERER_DLL
# error EDITOR_RENDERER_DLL with path to editor renderer DLL code is not defined
#endif
#define EDITOR_DLL_FILE_NAME ConvertNameToString(EDITOR_DLL)
#define EDITOR_RENDERER_DLL_FILE_NAME ConvertNameToString(EDITOR_RENDERER_DLL)

struct platform_clock
{
 u64 LastTSC;
 u64 CPU_TimerFreq;
};

platform_api Platform;

#endif //PLATFORM_SHARED_H
