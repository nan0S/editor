internal void
DrawCircle(v2 Position, f32 Radius,
           color Color, sf::Transform Transform,
           sf::RenderWindow *RenderWindow,
           f32 OutlineThickness,
           color OutlineColor)
{
   sf::CircleShape Circle = sf::CircleShape();
   
   Circle.setRadius(Radius);
   Circle.setFillColor(ColorToSFMLColor(Color));
   Circle.setOrigin(Radius, Radius);
   Circle.setPosition(Position.X, Position.Y);
   Circle.setOutlineThickness(OutlineThickness);
   Circle.setOutlineColor(ColorToSFMLColor(OutlineColor));
   
   RenderWindow->draw(Circle, Transform);
}

internal void
DrawSquare(v2 Position, f32 Side,
           color Color, sf::Transform Transform,
           sf::RenderWindow *RenderWindow)
{
   DrawRectangle(Position, V2(Side, Side), Rotation2DZero(),
                 Color, Transform, RenderWindow);
}

internal void
DrawRectangle(v2 Position, v2 Size, rotation_2d Rotation,
              color Color, sf::Transform Transform,
              sf::RenderWindow *RenderWindow)
{
   sf::RectangleShape Rectangle = sf::RectangleShape();
   
   Rectangle.setSize(V2ToVector2f(Size));
   Rectangle.setFillColor(ColorToSFMLColor(Color));
   Rectangle.setOrigin(0.5f * Size.X, 0.5f * Size.Y);
   Rectangle.setPosition(Position.X, Position.Y);
   Rectangle.setRotation(Rotation2DToDegrees(Rotation));
   
   RenderWindow->draw(Rectangle, Transform);
}

internal void
DrawLine(v2 BeginPoint, v2 EndPoint,
         f32 LineWidth, color Color,
         sf::Transform Transform,
         sf::RenderWindow *RenderWindow)
{
   v2 Position = 0.5f * (BeginPoint + EndPoint);
   v2 Line = EndPoint - BeginPoint;
   f32 Length = Norm(Line);
   v2 Size = V2(LineWidth, Length);
   // NOTE(hbr): Rotate 90 degrees clockwise, because our 0 degree rotation
   // corresponds to -90 degrees rotation in the real world
   rotation_2d Rotation = Rotate90DegreesClockwise(Rotation2DFromVector(Line));
   
   DrawRectangle(Position, Size, Rotation,
                 Color, Transform,
                 RenderWindow);
}

internal void
DrawTriangle(v2 P0, v2 P1, v2 P2,
             color Color, sf::Transform Transform,
             sf::RenderWindow *Window)
{
   sf::Vertex Vertices[3] = {};
   Vertices[0].position = V2ToVector2f(P0);
   Vertices[1].position = V2ToVector2f(P1);
   Vertices[2].position = V2ToVector2f(P2);
   Vertices[0].color = ColorToSFMLColor(Color);
   Vertices[1].color = ColorToSFMLColor(Color);
   Vertices[2].color = ColorToSFMLColor(Color);
   
   Window->draw(Vertices, 3, sf::Triangles, Transform);
}