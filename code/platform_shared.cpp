#include "base/base_core.cpp"
#include "base/base_string.cpp"
#include "base/base_os.cpp"
#include "base/base_arena.cpp"
#include "base/base_thread_ctx.cpp"
#include "base/base_hot_reload.cpp"

#include "editor_work_queue.cpp"
#include "editor_profiler.cpp"

internal void
Platform_MakeWorkQueues(work_queue *LowPriorityQueue, work_queue *HighPriorityQueue)
{
 u32 ProcCount = OS_ProcCount();
 
 u32 LowPriorityThreadCount = ProcCount * 1/4;
 u32 HighPriorityThreadCount = ProcCount * 3/4;
 LowPriorityThreadCount = ClampBot(LowPriorityThreadCount, 1);
 HighPriorityThreadCount = ClampBot(HighPriorityThreadCount, 1);
 
 WorkQueueInit(LowPriorityQueue, LowPriorityThreadCount);
 WorkQueueInit(HighPriorityQueue, HighPriorityThreadCount);
}

internal editor_memory
Platform_MakeEditorMemory(arena *PermamentArena, renderer_memory *RendererMemory,
                          work_queue *LowPriorityQueue, work_queue *HighPriorityQueue,
                          platform_api PlatformAPI, profiler *Profiler)
{
 editor_memory EditorMemory = {};
 EditorMemory.PermamentArena = PermamentArena;
 EditorMemory.MaxBufferCount = RendererMemory->Limits.MaxBufferCount;
 EditorMemory.MaxTextureCount = RendererMemory->Limits.MaxTextureCount;
 EditorMemory.RendererQueue = &RendererMemory->RendererQueue;
 EditorMemory.LowPriorityQueue = LowPriorityQueue;
 EditorMemory.HighPriorityQueue = HighPriorityQueue;
 EditorMemory.PlatformAPI = PlatformAPI;
 EditorMemory.Profiler = Profiler;
 
 return EditorMemory;
}

internal renderer_memory
Platform_MakeRendererMemory(arena *PermamentArena,
                            profiler *Profiler,
                            imgui_NewFrame *ImGuiNewFrame,
                            imgui_Render *ImGuiRender)
{
 renderer_memory RendererMemory = {};
 RendererMemory.PlatformAPI = Platform;
 
 platform_renderer_limits *Limits = &RendererMemory.Limits;
 // TODO(hbr): Revise those limits
 Limits->MaxTextureCount = 256;
 Limits->MaxBufferCount = 1024;
 
 renderer_transfer_queue *Queue = &RendererMemory.RendererQueue;
 Queue->TransferMemorySize = Megabytes(100);
 // TODO(hbr): use this value to test when memory renderer queue doesnt have space for an image
 //Queue->TransferMemorySize = 7680000 + 667152 - 1;
 Queue->TransferMemory = PushArrayNonZero(PermamentArena, Queue->TransferMemorySize, char);
 
 // TODO(hbr): Tweak these parameters
 RendererMemory.MaxLineCount = 1024;
 RendererMemory.LineBuffer = PushArrayNonZero(PermamentArena, RendererMemory.MaxLineCount, render_line);
 
 // TODO(hbr): Tweak these parameters
 RendererMemory.MaxCircleCount = 4096;
 RendererMemory.CircleBuffer = PushArrayNonZero(PermamentArena, RendererMemory.MaxCircleCount, render_circle);
 
 // TODO(hbr): Tweak these parameters
 RendererMemory.MaxImageCount = Limits->MaxTextureCount;
 RendererMemory.ImageBuffer = PushArrayNonZero(PermamentArena, RendererMemory.MaxImageCount, render_image);
 
 // TODO(hbr): Tweak these parameters
 RendererMemory.MaxVertexCount = 8 * 1024;
 RendererMemory.VertexBuffer = PushArrayNonZero(PermamentArena, RendererMemory.MaxVertexCount, render_vertex);
 
 RendererMemory.Profiler = Profiler;
 
 RendererMemory.ImGuiNewFrame = ImGuiNewFrame;
 RendererMemory.ImGuiRender = ImGuiRender;
 
 return RendererMemory;
}

internal platform_clock
Platform_MakeClock()
{
 platform_clock Clock = {};
 Clock.LastTSC = OS_ReadCPUTimer();
 Clock.CPU_TimerFreq = OS_CPUTimerFreq();
 
 return Clock;
}

internal f32
Platform_ClockFrame(platform_clock *Clock)
{
 u64 NowTSC = OS_ReadCPUTimer();
 f32 dtForFrame = Cast(f32)(NowTSC - Clock->LastTSC) / Clock->CPU_TimerFreq;
 Clock->LastTSC = NowTSC;
 
 return dtForFrame;
}

internal void
Platform_PrintDebugInputEvents(platform_input_output *Input)
{
 for (u32 EventIndex = 0;
      EventIndex < Input->EventCount;
      ++EventIndex)
 {
  platform_event *Event = Input->Events + EventIndex;
  string Name = PlatformEventTypeNames[Event->Type];
  string KeyName = PlatformKeyNames[Event->Key];
  //if (Event->Type != PlatformEvent_MouseMove)
  {
   OS_PrintDebugF("%S %S\n", Name, KeyName);
  }
 }
}

internal main_window_params
Platform_GetMainWindowInitialParams(u32 ScreenWidth, u32 ScreenHeight)
{
 u32 WindowWidth =  ScreenWidth * 1/2;
 //u32 WindowWidth =  ScreenWidth;
 //u32 WindowWidth =  ScreenWidth * 9/10;
 
 u32 WindowHeight = ScreenHeight * 1/2;
 //u32 WindowHeight = ScreenHeight;
 //u32 WindowHeight = ScreenHeight * 9/10;
 
 u32 WindowX = (ScreenWidth - WindowWidth) / 2;
 u32 WindowY = (ScreenHeight - WindowHeight) / 2;
 
 main_window_params Result = {};
 
 Result.LeftCornerP = V2U(WindowX, WindowY);
 Result.Dims = V2U(WindowWidth, WindowHeight);
 Result.UseDefault = (WindowWidth == 0 || WindowHeight == 0);
 
 Result.Title = "Parametric Curves Editor";
 
 return Result;
}

PLATFORM_SET_WINDOW_TITLE(PlatformSetWindowTitleStub) {}

#define X_Macro(Name, ImGuiName, ReturnType, Args, ArgNames) internal ReturnType ImGui##Name(Args){return ImGuiName(ArgNames);}
ImGuiFunctions
#undef X_Macro

platform_api Platform = {
 OS_Reserve,
 OS_Release,
 OS_Commit,
 ThreadCtxGetScratch,
 OS_OpenFileDialog,
 OS_SaveFileDialog,
 OS_ReadEntireFile,
 PlatformSetWindowTitleStub,
 WorkQueueAddEntry,
 WorkQueueCompleteAllWork,
 
 {
#define X_Macro(Name, ImGuiName, ReturnType, Args, ArgNames) ImGui##Name,
  ImGuiNonVariadicFunctions
#undef X_Macro
#define X_Macro(Name, ImGuiName, ReturnType, Args, ArgNames) ImGui::Name,
  ImGuiVariadicFunctions
#undef X_Macro
 },
};