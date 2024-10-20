#ifndef EDITOR_INPUT_H
#define EDITOR_INPUT_H

enum button
{
   Button_Left,
   Button_Right,
   Button_Middle,
   
   Button_Count,
};

struct button_state
{
   b32 WasPressed;
   b32 Pressed;
   v2s PressPosition;
   v2s ReleasePosition;
};

enum key
{
   Key_S,
   Key_W,
   Key_E,
   Key_O,
   Key_N,
   Key_Q,
   Key_LeftShift,
   Key_LeftCtrl,
   Key_Esc,
   Key_Tab,
   
   Key_Count,
};

typedef u64 modifier_flags;
enum
{
   Modifier_Shift = (1<<0),
   Modifier_Alt   = (1<<1),
   Modifier_Ctrl  = (1<<2),
};

struct key_state
{
   b32 WasPressed;
   b32 Pressed;
   modifier_flags ModifierFlags;
};

struct user_input
{
   button_state Buttons[Button_Count];
   key_state Keys[Key_Count];
   
   v2s MousePosition;
   v2s MouseLastPosition;
   
   f32 MouseWheelDelta;
   
   u64 WindowWidth;
   u64 WindowHeight;
   
   f32 dtForFrame;
};

// TODO(hbr): Rethink how mouse inputs should be handled. In particular when I press the mouse down
// I want the control point to be selected right away. Also ScreenPointsAreClose is probably nonsense.
// I don't want that - user can actually manage to click and release the button in place no problem
// so checking whether those two places are "close" is unnecessary.
internal b32
KeyPressed(user_input *Input, key Key, modifier_flags Flags)
{
   key_state *State = Input->Keys + Key;
   b32 JustPressed = (State->Pressed && !State->WasPressed);
   b32 WithModifiers = (State->ModifierFlags == Flags);
   b32 Result = (JustPressed && WithModifiers);
   
   return Result;
}

internal b32
IsKeyPressed(user_input *Input, key Key)
{
   key_state *State = Input->Keys + Key;
   b32 Result = State->Pressed;
   return Result;
}

internal user_input UserInputMake(v2s MousePosition, u64 WindowWidth, u64 WindowHeight);
internal void HandleEvents(sf::RenderWindow *Window, user_input *UserInput);

#endif //EDITOR_INPUT_H
