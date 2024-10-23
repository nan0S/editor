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
#include "editor_debug.h"
#include "editor.h"

#include "editor_profiler.cpp"
#include "editor_math.cpp"
#include "editor_renderer.cpp"
#include "editor_ui.cpp"
#include "editor_entity.cpp"
#include "editor_debug.cpp"
#include "editor_entity2.cpp"

#include "editor_sfml_renderer.h"
#include "editor_sfml_renderer.cpp"
#include "editor.cpp"

internal key
KeyFromSFMLKeyCode(sf::Keyboard::Key KeyCode)
{
   switch (KeyCode)
   {
      case sf::Keyboard::Escape:   { return Key_Esc;       } break;
      case sf::Keyboard::Q:        { return Key_Q;         } break;
      case sf::Keyboard::S:        { return Key_S;         } break;
      case sf::Keyboard::O:        { return Key_O;         } break;
      case sf::Keyboard::N:        { return Key_N;         } break;
      case sf::Keyboard::W:        { return Key_W;         } break;
      case sf::Keyboard::E:        { return Key_E;         } break;
      case sf::Keyboard::LShift:   { return Key_LeftShift; } break;
      case sf::Keyboard::LControl: { return Key_LeftCtrl;  } break;
      case sf::Keyboard::Tab:      { return Key_Tab;       } break;
      
      default: {} break;
   }
   
   return Key_Count;
}

internal void
HandleEvents(sf::RenderWindow *Window, editor_input *Input)
{
   for (u64 ButtonIndex = 0;
        ButtonIndex < ArrayCount(Input->Buttons);
        ++ButtonIndex)
   {
      Input->Buttons[ButtonIndex].WasPressed =
         Input->Buttons[ButtonIndex].Pressed;
   }
   
   for (u64 KeyIndex = 0;
        KeyIndex < ArrayCount(Input->Keys);
        ++KeyIndex)
   {
      Input->Keys[KeyIndex].WasPressed =
         Input->Keys[KeyIndex].Pressed;
   }
   
   Input->MouseLastPosition = Input->MousePosition;
   Input->MouseWheelDelta = 0.0f;
   
   ImGuiIO &ImGuiInput = ImGui::GetIO();
   sf::Event Event;
   while (Window->pollEvent(Event))
   {
      ImGui::SFML::ProcessEvent(*Window, Event);
      
      switch (Event.type)
      {
         case sf::Event::Closed: { Window->close(); } break;
         
         case sf::Event::KeyPressed: {
            key Key = KeyFromSFMLKeyCode(Event.key.code);
            
            if (Key != Key_Count)
            {
               modifier_flags ModifierFlags = 0;
               if (Event.key.shift)   ModifierFlags |= Modifier_Shift;
               if (Event.key.alt)     ModifierFlags |= Modifier_Alt;
               if (Event.key.control) ModifierFlags |= Modifier_Ctrl;
               
               Input->Keys[Key].Pressed = true;
               Input->Keys[Key].ModifierFlags = ModifierFlags;
            }
         } break;
         
         case sf::Event::KeyReleased: {
            key Key = KeyFromSFMLKeyCode(Event.key.code);
            
            if (Key != Key_Count)
            {
               Input->Keys[Key].Pressed = false;
               Input->Keys[Key].ModifierFlags = 0;
            }
         } break;
         
         case sf::Event::MouseButtonPressed: {
            if (!ImGuiInput.WantCaptureMouse)
            {
               switch (Event.mouseButton.button)
               {
                  case sf::Mouse::Left: {
                     button_state *LeftButton = &Input->Buttons[Button_Left];
                     LeftButton->Pressed = true;
                     LeftButton->PressPosition = V2S32(Event.mouseButton.x,
                                                       Event.mouseButton.y);
                  } break;
                  
                  case sf::Mouse::Right: {
                     button_state *RightButton = &Input->Buttons[Button_Right];
                     RightButton->Pressed = true;
                     RightButton->PressPosition = V2S32(Event.mouseButton.x,
                                                        Event.mouseButton.y);
                  } break;
                  
                  case sf::Mouse::Middle: {
                     button_state *MiddleButton = &Input->Buttons[Button_Middle];
                     MiddleButton->Pressed = true;
                     MiddleButton->PressPosition = V2S32(Event.mouseButton.x,
                                                         Event.mouseButton.y);
                  } break;
                  
                  default: {} break;
               }
            }
         } break;
         
         case sf::Event::MouseButtonReleased: {
            switch (Event.mouseButton.button)
            {
               case sf::Mouse::Left: {
                  button_state *LeftButton = &Input->Buttons[Button_Left];
                  LeftButton->Pressed = false;
                  LeftButton->ReleasePosition = V2S32(Event.mouseButton.x,
                                                      Event.mouseButton.y);
               } break;
               
               case sf::Mouse::Right: {
                  button_state *RightButton = &Input->Buttons[Button_Right];
                  RightButton->Pressed = false;
                  RightButton->ReleasePosition = V2S32(Event.mouseButton.x,
                                                       Event.mouseButton.y);
               } break;
               
               case sf::Mouse::Middle: {
                  button_state *MiddleButton = &Input->Buttons[Button_Middle];
                  MiddleButton->Pressed = false;
                  MiddleButton->ReleasePosition = V2S32(Event.mouseButton.x,
                                                        Event.mouseButton.y);
               } break;
               
               default: {} break;
            }
         } break;
         
         case sf::Event::MouseMoved: {
            Input->MousePosition = V2S32(Event.mouseMove.x,
                                         Event.mouseMove.y);
         } break;
         
         case sf::Event::MouseWheelScrolled: {
            Input->MouseWheelDelta += Event.mouseWheelScroll.delta;
         } break;
         
         default: {} break;
      }
   }
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
         
         editor_input Input_ = {};
         editor_input *Input = &Input_;
         sf::Clock Clock;
         
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
               
               HandleEvents(Window, Input);
               
               render_frame *Frame = SFMLBeginFrame(Renderer);
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