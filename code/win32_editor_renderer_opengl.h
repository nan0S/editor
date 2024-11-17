#ifndef WIN32_EDITOR_RENDERER_OPENGL_H
#define WIN32_EDITOR_RENDERER_OPENGL_H

struct opengl
{
 platform_renderer PlatformHeader;
 
 render_frame RenderFrame;
 
 render_command *CommandBuffer;
 u32 MaxCommandCount;
 
 u32 MaxTextureCount;
 GLuint *Textures;
 
 HWND Window;
};

#endif //WIN32_EDITOR_RENDERER_OPENGL_H
