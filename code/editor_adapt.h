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

#endif //EDITOR_ADAPT_H
