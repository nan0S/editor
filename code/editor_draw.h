#ifndef EDITOR_DRAW_H
#define EDITOR_DRAW_H

#if BUILD_DEBUG
#define DEBUG_DRAW_POINT(Position, Animate, RenderWindow) \
DrawCircle(Position, 0.01f, ColorMake(255, 10, 143), Animate, RenderWindow)
#else
#define DEBUG_DRAW_POINT(Position, Animate, RenderWindow)
#endif

function void DrawCircle(v2f32 Position, f32 Radius,
                         color Color, sf::Transform Transform,
                         sf::RenderWindow *RenderWindow,
                         f32 OutlineThickness = 0.0f,
                         color OutlineColor = ColorMake(0, 0, 0));
function void DrawSquare(v2f32 Position, f32 Side,
                         color Color, sf::Transform Transform,
                         sf::RenderWindow *RenderWindow);
function void DrawRectangle(v2f32 Position, v2f32 Size, rotation_2d Rotation,
                            color Color, sf::Transform Transform,
                            sf::RenderWindow *RenderWindow);
function void DrawLine(v2f32 BeginPoint, v2f32 EndPoint,
                       f32 LineWidth, color Color,
                       sf::Transform Transform,
                       sf::RenderWindow *RenderWindow);
function void DrawTriangle(v2f32 P0, v2f32 P1, v2f32 P2,
                           color Color, sf::Transform Transform,
                           sf::RenderWindow *Window);

#endif //EDITOR_DRAW_H
