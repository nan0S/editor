#ifndef WIN32_EDITOR_RENDERER_OPENGL_H
#define WIN32_EDITOR_RENDERER_OPENGL_H

struct opengl
{
 render_frame RenderFrame;
 
 u32 MaxTextureCount;
 GLuint *Textures;
 
 HWND Window;
};

#endif //WIN32_EDITOR_RENDERER_OPENGL_H
