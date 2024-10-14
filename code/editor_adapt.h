#ifndef EDITOR_ADAPT_H
#define EDITOR_ADAPT_H

internal inline sf::Vector2f
V2ToVector2f(v2 V)
{
   sf::Vector2f Result = {};
   Result.x = V.X;
   Result.y = V.Y;
   
   return Result;
}

internal inline sf::Vector2i
V2S32ToVector2i(v2s V)
{
   sf::Vector2i Result = {};
   Result.x = V.X;
   Result.y = V.Y;
   
   return Result;
}

internal inline sf::Color
ColorToSFMLColor(v4 Color)
{
   sf::Color Result = {};
   Result.r = Cast(u8)(255 * ClampTop(Color.R, 1.0f));
   Result.g = Cast(u8)(255 * ClampTop(Color.G, 1.0f));
   Result.b = Cast(u8)(255 * ClampTop(Color.B, 1.0f));
   Result.a = Cast(u8)(255 * ClampTop(Color.A, 1.0f));
   
   return Result;
}

#endif //EDITOR_ADAPT_H
