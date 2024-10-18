#ifndef EDITOR_DRAW_H
#define EDITOR_DRAW_H

// TODO(hbr): Probably remove
#if BUILD_DEBUG
#define DEBUG_DRAW_POINT(Position, Animate, RenderWindow) \
PushCircle(Position, 0.01f, RGBA_Color(255, 10, 143), Animate, RenderWindow)
#else
#define DEBUG_DRAW_POINT(Position, Animate, RenderWindow)
#endif

#endif //EDITOR_DRAW_H
