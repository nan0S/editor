#ifndef X11_EDITOR_RENDERER_OPENGL_H
#define X11_EDITOR_RENDERER_OPENGL_H

typedef GLXContext func_glXCreateContextAttribsARB(Display *dpy, GLXFBConfig config, GLXContext share_context, Bool direct, const int *attrib_list);

struct x11_opengl_renderer
{
 Display *Display;
 Window X11Window;
};

#endif //X11_EDITOR_RENDERER_OPENGL_H