#ifndef EDITOR_RENDERER_SFML_H
#define EDITOR_RENDERER_SFML_H

struct sfml_renderer
{
 platform_renderer PlatformHeader;
 
 sf::RenderWindow *Window;
 
 render_frame Frame;
 render_command CommandBuffer[4096];
 
 u64 MaxTextureCount;
 GLuint *Textures;
};

#endif //EDITOR_RENDERER_SFML_H
