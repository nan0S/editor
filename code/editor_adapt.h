#ifndef EDITOR_ADAPT_H
#define EDITOR_ADAPT_H

internal inline sf::Vector2f
V2F32ToVector2f(v2f32 V)
{
   sf::Vector2f Result = {};
   Result.x = V.X;
   Result.y = V.Y;
   
   return Result;
}

internal inline sf::Vector2i
V2S32ToVector2i(v2s32 V)
{
   sf::Vector2i Result = {};
   Result.x = V.X;
   Result.y = V.Y;
   
   return Result;
}

internal inline sf::Color
ColorToSFMLColor(color Color)
{
   sf::Color Result = {};
   Result.r = Color.R;
   Result.g = Color.G;
   Result.b = Color.B;
   Result.a = Color.A;
   
   return Result;
}

namespace ImGui
{
   internal inline bool ColorEdit3(char const *Label, color *Color)
   {
      f32 ColorAsArray[3] = {
         Color->R / 255.0f,
         Color->G / 255.0f,
         Color->B / 255.0f,
      };
      
      bool Result = ColorEdit3(Label, ColorAsArray);
      
      *Color = ColorMake(Cast(u8)(255 * ColorAsArray[0]),
                         Cast(u8)(255 * ColorAsArray[1]),
                         Cast(u8)(255 * ColorAsArray[2]));
      
      return Result;
   }
   
   internal inline bool ColorEdit4(char const *Label, color *Color)
   {
      f32 ColorAsArray[4] = {
         Color->R / 255.0f,
         Color->G / 255.0f,
         Color->B / 255.0f,
         Color->A / 255.0f,
      };
      
      bool Result = ColorEdit4(Label, ColorAsArray);
      
      *Color = ColorMake(Cast(u8)(255 * ColorAsArray[0]),
                         Cast(u8)(255 * ColorAsArray[1]),
                         Cast(u8)(255 * ColorAsArray[2]),
                         Cast(u8)(255 * ColorAsArray[3]));
      
      return Result;
   }
}

#endif //EDITOR_ADAPT_H
