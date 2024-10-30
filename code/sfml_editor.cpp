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

#include "third_party/sfml/include/SFML/Graphics.hpp"

#define EDITOR_PROFILER 1
#include "editor_profiler.h"
#include "editor_math.h"
#include "editor_ui.h"
#include "editor_sort.h"
#include "editor_entity2.h"
#include "editor_adapt.h"
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

#include "editor_sfml_renderer.h"
#include "editor_sfml_renderer.cpp"

#include "sfml_editor.h"

#include "editor.cpp"

internal platform_key
SFMLKeyToPlatformKey(sf::Keyboard::Key Key)
{
   switch (Key)
   {
      case sf::Keyboard::F1:  {return PlatformKey_F1;}break;
      case sf::Keyboard::F2:  {return PlatformKey_F2;}break;
      case sf::Keyboard::F3:  {return PlatformKey_F3;}break;
      case sf::Keyboard::F4:  {return PlatformKey_F4;}break;
      case sf::Keyboard::F5:  {return PlatformKey_F5;}break;
      case sf::Keyboard::F6:  {return PlatformKey_F6;}break;
      case sf::Keyboard::F7:  {return PlatformKey_F7;}break;
      case sf::Keyboard::F8:  {return PlatformKey_F8;}break;
      case sf::Keyboard::F9:  {return PlatformKey_F9;}break;
      case sf::Keyboard::F10: {return PlatformKey_F10;}break;
      case sf::Keyboard::F11: {return PlatformKey_F11;}break;
      case sf::Keyboard::F12: {return PlatformKey_F12;}break;
      
      case sf::Keyboard::A: {return PlatformKey_A;}break;
      case sf::Keyboard::B: {return PlatformKey_B;}break;
      case sf::Keyboard::C: {return PlatformKey_C;}break;
      case sf::Keyboard::D: {return PlatformKey_D;}break;
      case sf::Keyboard::E: {return PlatformKey_E;}break;
      case sf::Keyboard::F: {return PlatformKey_F;}break;
      case sf::Keyboard::G: {return PlatformKey_G;}break;
      case sf::Keyboard::H: {return PlatformKey_H;}break;
      case sf::Keyboard::I: {return PlatformKey_I;}break;
      case sf::Keyboard::J: {return PlatformKey_J;}break;
      case sf::Keyboard::K: {return PlatformKey_K;}break;
      case sf::Keyboard::L: {return PlatformKey_L;}break;
      case sf::Keyboard::M: {return PlatformKey_M;}break;
      case sf::Keyboard::N: {return PlatformKey_N;}break;
      case sf::Keyboard::O: {return PlatformKey_O;}break;
      case sf::Keyboard::P: {return PlatformKey_P;}break;
      case sf::Keyboard::Q: {return PlatformKey_Q;}break;
      case sf::Keyboard::R: {return PlatformKey_R;}break;
      case sf::Keyboard::S: {return PlatformKey_S;}break;
      case sf::Keyboard::T: {return PlatformKey_T;}break;
      case sf::Keyboard::U: {return PlatformKey_U;}break;
      case sf::Keyboard::V: {return PlatformKey_V;}break;
      case sf::Keyboard::W: {return PlatformKey_W;}break;
      case sf::Keyboard::X: {return PlatformKey_X;}break;
      case sf::Keyboard::Y: {return PlatformKey_Y;}break;
      case sf::Keyboard::Z: {return PlatformKey_Z;}break;
      
      case sf::Keyboard::Escape:   {return PlatformKey_Escape;}break;
      case sf::Keyboard::LShift:   {return PlatformKey_LeftShift;}break;
      case sf::Keyboard::RShift:   {return PlatformKey_RightShift;}break;
      case sf::Keyboard::LControl: {return PlatformKey_LeftCtrl;}break;
      case sf::Keyboard::RControl: {return PlatformKey_RightCtrl;}break;
      case sf::Keyboard::LAlt:     {return PlatformKey_LeftAlt;}break;
      case sf::Keyboard::RAlt:     {return PlatformKey_RightAlt;}break;
      case sf::Keyboard::Space:    {return PlatformKey_Space;}break;
      case sf::Keyboard::Tab:      {return PlatformKey_Tab;}break;
      
      default: {}break;
   }
   
   return PlatformKey_Unknown;
}

internal platform_key
SFMLButtonToPlatformKey(sf::Mouse::Button Button)
{
   switch (Button)
   {
      case sf::Mouse::Left: {return PlatformKey_LeftMouseButton;}break;
      case sf::Mouse::Right: {return PlatformKey_RightMouseButton;}break;
      case sf::Mouse::Middle: {return PlatformKey_MiddleMouseButton;}break;
      default: {}break;
   }
   
   return PlatformKey_Unknown;
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
            Input->Pressed[Key] = Pressed;
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
            Input->Pressed[Key] = Pressed;
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
   
   sf::Vector2i MouseP = sf::Mouse::getPosition();
   Input->ClipSpaceMouseP = SFMLScreenToClip(MouseP.x, MouseP.y, WindowDim);
}

int main()
{
   // TODO(hbr): Move to editor code, or move out entirely
   InitThreadCtx();
   InitOS();
   InitProfiler();
   
   arena *PermamentArena = AllocArena();
   
   sf::VideoMode VideoMode = sf::VideoMode::getDesktopMode();
   sf::ContextSettings ContextSettings = sf::ContextSettings();
   ContextSettings.antialiasingLevel = 4;
   sf::RenderWindow Window_(VideoMode, WINDOW_TITLE, sf::Style::Default, ContextSettings);
   sf::RenderWindow *Window = &Window_;
   
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
         sfml_renderer *Renderer = InitSFMLRenderer(PermamentArena, Window);
         
         sf::Clock Clock;
         sfml_platform_input SFMLInput = {};
         platform_input Input_ = {};
         platform_input *Input = &Input_;
         
         editor_memory Memory_ = {};
         editor_memory *Memory = &Memory_;
         Memory->PermamentMemorySize = Megabytes(128);
         Memory->PermamentMemory = PushSize(PermamentArena, Memory->PermamentMemorySize);
         
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