#ifndef EDITOR_DRAW_H
#define EDITOR_DRAW_H

#if BUILD_DEBUG
#define DEBUG_DRAW_POINT(Position, Animate, RenderWindow) \
DrawCircle(Position, 0.01f, MakeColor(255, 10, 143), Animate, RenderWindow)
#else
#define DEBUG_DRAW_POINT(Position, Animate, RenderWindow)
#endif

internal void DrawCircle(v2 Position, f32 Radius,
                         color Color, sf::Transform Transform,
                         sf::RenderWindow *RenderWindow,
                         f32 OutlineThickness = 0.0f,
                         color OutlineColor = MakeColor(0, 0, 0));
internal void DrawSquare(v2 Position, f32 Side,
                         color Color, sf::Transform Transform,
                         sf::RenderWindow *RenderWindow);
internal void DrawRectangle(v2 Position, v2 Size, rotation_2d Rotation,
                            color Color, sf::Transform Transform,
                            sf::RenderWindow *RenderWindow);
internal void DrawLine(v2 BeginPoint, v2 EndPoint,
                       f32 LineWidth, color Color,
                       sf::Transform Transform,
                       sf::RenderWindow *RenderWindow);
internal void DrawTriangle(v2 P0, v2 P1, v2 P2,
                           color Color, sf::Transform Transform,
                           sf::RenderWindow *Window);

#endif //EDITOR_DRAW_H
