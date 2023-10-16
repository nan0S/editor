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
   
   v2s32 PressPosition;
   v2s32 ReleasePosition;
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
   Key_ESC,
   
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
   
   v2s32 MousePosition;
   v2s32 MouseLastPosition;
   
   f32 MouseWheelDelta;
   
   u64 WindowWidth;
   u64 WindowHeight;
};

function user_input UserInputMake(v2s32 MousePosition, u64 WindowWidth, u64 WindowHeight);
function void HandleEvents(sf::RenderWindow *Window, user_input *UserInput);

#endif //EDITOR_INPUT_H
