#ifndef EDITOR_DRAW_H
#define EDITOR_DRAW_H

#if BUILD_DEBUG
#define DEBUG_DRAW_POINT(Position, Animate, RenderWindow) \
DrawCircle(Position, 0.01f, RGBA_Color(255, 10, 143), Animate, RenderWindow)
#else
#define DEBUG_DRAW_POINT(Position, Animate, RenderWindow)
#endif

internal void DrawCircle(v2 Position, f32 Radius,
                         v4 Color, sf::Transform Transform,
                         sf::RenderWindow *RenderWindow,
                         f32 OutlineThickness = 0.0f,
                         v4 OutlineColor = RGBA_Color(0, 0, 0));
internal void DrawSquare(v2 Position, f32 Side,
                         v4 Color, sf::Transform Transform,
                         sf::RenderWindow *RenderWindow);
internal void DrawRectangle(v2 Position, v2 Size, rotation_2d Rotation,
                            v4 Color, sf::Transform Transform,
                            sf::RenderWindow *RenderWindow);
internal void DrawLine(v2 BeginPoint, v2 EndPoint,
                       f32 LineWidth, v4 Color,
                       sf::Transform Transform,
                       sf::RenderWindow *RenderWindow);
internal void DrawTriangle(v2 P0, v2 P1, v2 P2,
                           v4 Color, sf::Transform Transform,
                           sf::RenderWindow *Window);

#endif //EDITOR_DRAW_H
