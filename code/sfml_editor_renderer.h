#ifndef SFML_EDITOR_RENDERER_H
#define SFML_EDITOR_RENDERER_H

#define SFML_RENDERER_INIT(Name) platform_renderer *Name(sf::RenderWindow *Window)

struct sfml_renderer
{
 render_frame Frame;
 sf::RenderWindow *Window;
 render_command CommandBuffer[4096];
};

#endif //SFML_EDITOR_RENDERER_H
