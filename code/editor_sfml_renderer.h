#ifndef EDITOR_SFML_RENDERER_H
#define EDITOR_SFML_RENDERER_H

struct sfml_renderer
{
 render_frame Frame;
 sf::RenderWindow *Window;
 render_command CommandBuffer[4096];
};

internal sfml_renderer *InitSFMLRenderer(arena *Arena, sf::RenderWindow *Window);
internal render_frame *SFMLBeginFrame(sfml_renderer *Renderer);
internal void SFMLEndFrame(sfml_renderer *Renderer, render_frame *Frame);

#endif //EDITOR_SFML_RENDERER_H
