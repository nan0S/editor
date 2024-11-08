// TODO(hbr): what the fuck is this
#pragma warning(1 : 4062)
#include "imgui_inc.h"

#include "editor_base.h"
#include "editor_memory.h"
#include "editor_string.h"
#include "editor_os.h"
#include "editor_renderer.h"
#include "editor_platform.h"

#include "editor_base.cpp"
#include "editor_memory.cpp"
#include "editor_string.cpp"
#include "editor_os.cpp"

#define EDITOR_PROFILER 1
#include "editor_profiler.h"
#include "editor_math.h"
#include "editor_ui.h"
#include "editor_sort.h"
#include "editor_entity2.h"
#include "editor_entity.h"
#include "editor.h"

#include "editor_profiler.cpp"
#include "editor_math.cpp"
#include "editor_renderer.cpp"
#include "editor_ui.cpp"
#include "editor_entity.cpp"
#include "editor_entity2.cpp"

#define STB_IMAGE_IMPLEMENTATION
#include "third_party/stb/stb_image.h"

#include "third_party/sfml/include/SFML/Graphics.hpp"

#include "sfml_editor_renderer.h"
#include "sfml_editor.h"

#include "editor.cpp"

RENDERER_BEGIN_FRAME(SFMLBeginFrame);
RENDERER_END_FRAME(SFMLEndFrame);
SFML_RENDERER_INIT(SFMLRendererInit);

global sf::Keyboard::Key SFMLKeys[] =
{
 sf::Keyboard::F1,
 sf::Keyboard::F2,
 sf::Keyboard::F3,
 sf::Keyboard::F4,
 sf::Keyboard::F5,
 sf::Keyboard::F6,
 sf::Keyboard::F7,
 sf::Keyboard::F8,
 sf::Keyboard::F9,
 sf::Keyboard::F10,
 sf::Keyboard::F11,
 sf::Keyboard::F12,
 
 sf::Keyboard::A,
 sf::Keyboard::B,
 sf::Keyboard::C,
 sf::Keyboard::D,
 sf::Keyboard::E,
 sf::Keyboard::F,
 sf::Keyboard::G,
 sf::Keyboard::H,
 sf::Keyboard::I,
 sf::Keyboard::J,
 sf::Keyboard::K,
 sf::Keyboard::L,
 sf::Keyboard::M,
 sf::Keyboard::N,
 sf::Keyboard::O,
 sf::Keyboard::P,
 sf::Keyboard::Q,
 sf::Keyboard::R,
 sf::Keyboard::S,
 sf::Keyboard::T,
 sf::Keyboard::U,
 sf::Keyboard::V,
 sf::Keyboard::W,
 sf::Keyboard::X,
 sf::Keyboard::Y,
 sf::Keyboard::Z,
 
 sf::Keyboard::Escape,
 sf::Keyboard::LShift,
 sf::Keyboard::RShift,
 sf::Keyboard::LControl,
 sf::Keyboard::RControl,
 sf::Keyboard::LAlt,
 sf::Keyboard::RAlt,
 sf::Keyboard::Space,
 sf::Keyboard::Tab,
};

global platform_key SFMLPlatformKeys[] =
{
 PlatformKey_F1,
 PlatformKey_F2,
 PlatformKey_F3,
 PlatformKey_F4,
 PlatformKey_F5,
 PlatformKey_F6,
 PlatformKey_F7,
 PlatformKey_F8,
 PlatformKey_F9,
 PlatformKey_F10,
 PlatformKey_F11,
 PlatformKey_F12,
 
 PlatformKey_A,
 PlatformKey_B,
 PlatformKey_C,
 PlatformKey_D,
 PlatformKey_E,
 PlatformKey_F,
 PlatformKey_G,
 PlatformKey_H,
 PlatformKey_I,
 PlatformKey_J,
 PlatformKey_K,
 PlatformKey_L,
 PlatformKey_M,
 PlatformKey_N,
 PlatformKey_O,
 PlatformKey_P,
 PlatformKey_Q,
 PlatformKey_R,
 PlatformKey_S,
 PlatformKey_T,
 PlatformKey_U,
 PlatformKey_V,
 PlatformKey_W,
 PlatformKey_X,
 PlatformKey_Y,
 PlatformKey_Z,
 
 PlatformKey_Escape,
 PlatformKey_LeftShift,
 PlatformKey_RightShift,
 PlatformKey_LeftCtrl,
 PlatformKey_RightCtrl,
 PlatformKey_LeftAlt,
 PlatformKey_RightAlt,
 PlatformKey_Space,
 PlatformKey_Tab,
};

global sf::Mouse::Button SFMLButtons[] =
{
 sf::Mouse::Left,
 sf::Mouse::Right,
 sf::Mouse::Middle,
};

global platform_key SFMLPlatformButtons[] =
{
 PlatformKey_LeftMouseButton,
 PlatformKey_RightMouseButton,
 PlatformKey_MiddleMouseButton,
};

internal platform_key
SFMLKeyToPlatformKey(sf::Keyboard::Key Key)
{
 platform_key Result = PlatformKey_Unknown;
 for (u64 Index = 0;
      Index < ArrayCount(SFMLKeys);
      ++Index)
 {
  if (SFMLKeys[Index] == Key)
  {
   Result = SFMLPlatformKeys[Index];
   break;
  }
 }
 
 return Result;
}

internal platform_key
SFMLButtonToPlatformKey(sf::Mouse::Button Button)
{
 platform_key Result = PlatformKey_Unknown;
 for (u64 Index = 0;
      Index < ArrayCount(SFMLButtons);
      ++Index)
 {
  if (SFMLButtons[Index] == Button)
  {
   Result = SFMLPlatformButtons[Index];
   break;
  }
 }
 
 return Result;
}

internal platform_event *
SFMLPushPlatformEvent(sfml_platform_input *Input, platform_event_type Type)
{
 platform_event *Event = 0;
 if (Input->EventCount < SFML_PLATFORM_INPUT_MAX_EVENT_COUNT)
 {
  Event = Input->Events + Input->EventCount++;
  Event->Type = Type;
 }
 return Event;
}

internal v2
SFMLScreenToClip(int X, int Y, v2u WindowDim)
{
 v2 Result = V2(+(2.0f * X / WindowDim.X - 1.0f),
                -(2.0f * Y / WindowDim.Y - 1.0f));
 return Result;
}

internal void
SFMLHandleInput(platform_input *Input,
                sfml_platform_input *SFMLInput,
                sf::RenderWindow *Window,
                v2u WindowDim)
{
 ImGuiIO &ImGuiInput = ImGui::GetIO();
 sf::Event SFMLEvent;
 while (Window->pollEvent(SFMLEvent))
 {
  ImGui::SFML::ProcessEvent(*Window, SFMLEvent);
  switch (SFMLEvent.type)
  {
   case sf::Event::Closed: { Window->close(); }break;
   
   case sf::Event::KeyPressed:
   case sf::Event::KeyReleased: {
    b32 Pressed = (SFMLEvent.type == sf::Event::KeyPressed);
    platform_key Key = SFMLKeyToPlatformKey(SFMLEvent.key.code);
    platform_event *Event = SFMLPushPlatformEvent(SFMLInput, (Pressed ? PlatformEvent_Press : PlatformEvent_Release));
    if (Event)
    {
     Event->Key = Key;
     if (SFMLEvent.key.shift) Event->Flags |= PlatformEventFlag_Shift;
     if (SFMLEvent.key.alt) Event->Flags |= PlatformEventFlag_Alt;
     if (SFMLEvent.key.control) Event->Flags |= PlatformEventFlag_Ctrl;
    }
   }break;
   
   case sf::Event::MouseButtonPressed:
   case sf::Event::MouseButtonReleased: {
    b32 Pressed = (SFMLEvent.type == sf::Event::MouseButtonPressed);
    platform_key Key = SFMLButtonToPlatformKey(SFMLEvent.mouseButton.button);
    if (!ImGuiInput.WantCaptureMouse)
    {
     platform_event *Event = SFMLPushPlatformEvent(SFMLInput, (Pressed ? PlatformEvent_Press : PlatformEvent_Release));
     if (Event)
     {
      Event->Key = Key;
      Event->ClipSpaceMouseP = SFMLScreenToClip(SFMLEvent.mouseButton.x,
                                                SFMLEvent.mouseButton.y,
                                                WindowDim);
     }
    }
   } break;
   
   case sf::Event::MouseMoved: {
    platform_event *Event = SFMLPushPlatformEvent(SFMLInput, PlatformEvent_MouseMove);
    if (Event)
    {
     Event->ClipSpaceMouseP = SFMLScreenToClip(SFMLEvent.mouseMove.x,
                                               SFMLEvent.mouseMove.y,
                                               WindowDim);
    }
   } break;
   
   case sf::Event::MouseWheelScrolled: {
    platform_event *Event = SFMLPushPlatformEvent(SFMLInput, PlatformEvent_Scroll);
    if (Event)
    {
     Event->ScrollDelta = SFMLEvent.mouseWheelScroll.delta;
    }
   } break;
   
   default: {} break;
  }
 }
 
 // NOTE(hbr): Just to be safe and also avoid bugs, query the state of all the keys/buttons.
 // Don't rely too much on state-dependent, event-processing loop above for final key/button state.
 // It could happen that some events are not processed for whatever reason (other window has been opened)
 // and this event has not been sent to our main window but to that window instead.
 for (u64 Index = 0;
      Index < ArrayCount(SFMLKeys);
      ++Index)
 {
  sf::Keyboard::Key SFMLKey = SFMLKeys[Index];
  platform_key PlatformKey = SFMLPlatformKeys[Index];
  Input->Pressed[PlatformKey] = sf::Keyboard::isKeyPressed(SFMLKey);
 }
 for (u64 Index = 0;
      Index < ArrayCount(SFMLButtons);
      ++Index)
 {
  sf::Mouse::Button SFMLButton = SFMLButtons[Index];
  platform_key PlatformButton = SFMLPlatformButtons[Index];
  Input->Pressed[PlatformButton] = sf::Mouse::isButtonPressed(SFMLButton);
 }
 
 sf::Vector2i MouseP = sf::Mouse::getPosition();
 Input->ClipSpaceMouseP = SFMLScreenToClip(MouseP.x, MouseP.y, WindowDim);
}

PLATFORM_OPEN_FILE_DIALOG(SFMLOpenFileDialog)
{
 platform_file_dialog_result Result = {};
 
 u64 Count = 256;
 char *Buffer = PushArrayNonZero(Arena, Count, char);
 
 HWND WindowHandle = GetActiveWindow();
 
 OPENFILENAME Open = {};
 Open.lStructSize = SizeOf(Open);
 Open.lpstrFile = Buffer;
 Open.hwndOwner = WindowHandle;
 if (Count > 0) Open.lpstrFile[0] = '\0';
 Open.nMaxFile = Count;
 Open.lpstrFilter =
  "All Files (*.*)\0" "*.*\0"
  "PNG (*.png)\0" "*.png\0"
  "JPEG (*.jpg *.jpeg *jpe)\0" "*.jpg;*.jpeg;*.jpe\0"
  "Windows BMP File (*.bmp)\0" "*.bmp\0"
  ;
 Open.nFilterIndex = 1;
 Open.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
 
 if (GetOpenFileName(&Open) == TRUE)
 {
  Result.Success = true;
  Result.FilePath = StrFromCStr(Buffer);
 }
 
 return Result;
}

int main()
{
 // TODO(hbr): Move to editor code, or move out entirely
 InitThreadCtx();
 InitOS();
 InitProfiler();
 
 sf::VideoMode VideoMode = sf::VideoMode::getDesktopMode();
 sf::ContextSettings ContextSettings = sf::ContextSettings();
 ContextSettings.antialiasingLevel = 4;
 sf::RenderWindow Window_(VideoMode, "Parametric Curves Editor", sf::Style::Default, ContextSettings);
 sf::RenderWindow *Window = &Window_;
 
 platform_renderer_limits Limits_ = {};
 platform_renderer_limits *Limits = &Limits_;
 Limits->MaxTextureQueueMemorySize = Megabytes(100);
 Limits->MaxTextureCount = 256;
 
#if 0
 //#if not(BUILD_DEBUG)
 Window.setFramerateLimit(60);
 //#endif
#endif
 
 if (Window->isOpen())
 {
  bool ImGuiInitSuccess = ImGui::SFML::Init(*Window, true);
  if (ImGuiInitSuccess)
  {
   arena *PermamentArena = AllocArena();
   
   platform_renderer *Renderer = SFMLRendererInit(PermamentArena, Limits, Window);
   
   sf::Clock Clock;
   sfml_platform_input SFMLInput = {};
   platform_input Input_ = {};
   platform_input *Input = &Input_;
   
   editor_memory Memory_ = {};
   editor_memory *Memory = &Memory_;
   Memory->PermamentArena = PermamentArena;
   Memory->TextureQueue = &Renderer->TextureQueue;
   Memory->MaxTextureCount = Limits->MaxTextureCount;
   Memory->PlatformOpenFileDialog = SFMLOpenFileDialog;
   
   b32 Running = true;
   while (Running)
   {
    if (!Window->isOpen())
    {
     Running = false;
    }
    else
    {
     auto SFMLDeltaTime = Clock.restart();
     Input->dtForFrame = SFMLDeltaTime.asSeconds();
     {
      TimeBlock("ImGui::SFML::Update");
      ImGui::SFML::Update(*Window, SFMLDeltaTime);
     }
     
     sf::Vector2u WindowDim_ = Window->getSize();
     v2u WindowDim = V2U(WindowDim_.x, WindowDim_.y);
     
     sfml_platform_input SFMLInput = {};
     SFMLHandleInput(Input, &SFMLInput, Window, WindowDim);
     
     Input->EventCount = SFMLInput.EventCount;
     Input->Events = SFMLInput.Events;
     
     render_frame *Frame = SFMLBeginFrame(Renderer, WindowDim);
     EditorUpdateAndRender(Memory, Input, Frame);
     SFMLEndFrame(Renderer, Frame);
     
     if (Input->QuitRequested)
     {
      Running = false;
     }
    }
   }
  }
  else
  {
   // TODO(hbr): Handle error
  }
 }
 else
 {
  // TODO(hbr): Handle error
 }
 
 return 0;
}