internal key
KeyFromSFMLKeyCode(sf::Keyboard::Key KeyCode)
{
   switch (KeyCode)
   {
      case sf::Keyboard::Escape:   { return Key_ESC;       } break;
      case sf::Keyboard::Q:        { return Key_Q;         } break;
      case sf::Keyboard::S:        { return Key_S;         } break;
      case sf::Keyboard::O:        { return Key_O;         } break;
      case sf::Keyboard::N:        { return Key_N;         } break;
      case sf::Keyboard::W:        { return Key_W;         } break;
      case sf::Keyboard::E:        { return Key_E;         } break;
      case sf::Keyboard::LShift:   { return Key_LeftShift; } break;
      case sf::Keyboard::LControl: { return Key_LeftCtrl; } break;
      
      default: {} break;
   }
   
   return Key_Count;
}

function void
HandleEvents(sf::RenderWindow *Window, user_input *UserInput)
{
   TimeFunction;
   
   for (u64 ButtonIndex = 0;
        ButtonIndex < ArrayCount(UserInput->Buttons);
        ++ButtonIndex)
   {
      UserInput->Buttons[ButtonIndex].WasPressed =
         UserInput->Buttons[ButtonIndex].Pressed;
   }
   
   for (u64 KeyIndex = 0;
        KeyIndex < ArrayCount(UserInput->Keys);
        ++KeyIndex)
   {
      UserInput->Keys[KeyIndex].WasPressed =
         UserInput->Keys[KeyIndex].Pressed;
   }
   
   UserInput->MouseLastPosition = UserInput->MousePosition;
   UserInput->MouseWheelDelta = 0.0f;
   
   ImGuiIO &ImGuiUserInput = ImGui::GetIO();
   sf::Event Event;
   while (Window->pollEvent(Event))
   {
      ImGui::SFML::ProcessEvent(*Window, Event);
      
      switch (Event.type)
      {
         case sf::Event::Closed: { Window->close(); } break;
         
         case sf::Event::Resized: {
            sf::Vector2u WindowSize = Window->getSize();
            UserInput->WindowWidth = Event.size.width;
            UserInput->WindowHeight = Event.size.height;
         } break;
         
         case sf::Event::KeyPressed: {
            key Key = KeyFromSFMLKeyCode(Event.key.code);
            
            if (Key != Key_Count)
            {
               modifier_flags ModifierFlags = 0;
               if (Event.key.shift)   ModifierFlags |= Modifier_Shift;
               if (Event.key.alt)     ModifierFlags |= Modifier_Alt;
               if (Event.key.control) ModifierFlags |= Modifier_Ctrl;
               
               UserInput->Keys[Key].Pressed = true;
               UserInput->Keys[Key].ModifierFlags = ModifierFlags;
            }
         } break;
         
         case sf::Event::KeyReleased: {
            key Key = KeyFromSFMLKeyCode(Event.key.code);
            
            if (Key != Key_Count)
            {
               UserInput->Keys[Key].Pressed = false;
               UserInput->Keys[Key].ModifierFlags = 0;
            }
         } break;
         
         case sf::Event::MouseButtonPressed: {
            if (!ImGuiUserInput.WantCaptureMouse)
            {
               switch (Event.mouseButton.button)
               {
                  case sf::Mouse::Left: {
                     button_state *LeftButton = &UserInput->Buttons[Button_Left];
                     LeftButton->Pressed = true;
                     LeftButton->PressPosition = V2S32(Event.mouseButton.x,
                                                       Event.mouseButton.y);
                  } break;
                  
                  case sf::Mouse::Right: {
                     button_state *RightButton = &UserInput->Buttons[Button_Right];
                     RightButton->Pressed = true;
                     RightButton->PressPosition = V2S32(Event.mouseButton.x,
                                                        Event.mouseButton.y);
                  } break;
                  
                  case sf::Mouse::Middle: {
                     button_state *MiddleButton = &UserInput->Buttons[Button_Middle];
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
                  button_state *LeftButton = &UserInput->Buttons[Button_Left];
                  LeftButton->Pressed = false;
                  LeftButton->ReleasePosition = V2S32(Event.mouseButton.x,
                                                      Event.mouseButton.y);
               } break;
               
               case sf::Mouse::Right: {
                  button_state *RightButton = &UserInput->Buttons[Button_Right];
                  RightButton->Pressed = false;
                  RightButton->ReleasePosition = V2S32(Event.mouseButton.x,
                                                       Event.mouseButton.y);
               } break;
               
               case sf::Mouse::Middle: {
                  button_state *MiddleButton = &UserInput->Buttons[Button_Middle];
                  MiddleButton->Pressed = false;
                  MiddleButton->ReleasePosition = V2S32(Event.mouseButton.x,
                                                        Event.mouseButton.y);
               } break;
               
               default: {} break;
            }
         } break;
         
         case sf::Event::MouseMoved: {
            UserInput->MousePosition = V2S32(Event.mouseMove.x,
                                             Event.mouseMove.y);
         } break;
         
         case sf::Event::MouseWheelScrolled: {
            UserInput->MouseWheelDelta += Event.mouseWheelScroll.delta;
         } break;
         
         default: {} break;
      }
   }
}