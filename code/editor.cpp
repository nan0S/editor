#include "editor.h"

#include "editor_base.cpp"
#include "editor_os.cpp"
#include "editor_profiler.cpp"
#include "editor_math.cpp"

#include "editor_file.cpp"

#include "editor_input.cpp"
#include "editor_entity.cpp"
#include "editor_project.cpp"
#include "editor_debug.cpp"
#include "editor_draw.cpp"
#include "editor_editor.cpp"

internal void
ReportError(char const *Fmt, ...)
{
   va_list Args;
   va_start(Args, Fmt);
   
   fprintf(stderr, "error: ");
   vfprintf(stderr, Fmt, Args);
   fprintf(stderr, "\n");
   
   va_end(Args);
}

internal void
Log(char const *Fmt, ...)
{
   va_list Args;
   va_start(Args, Fmt);
   
   fprintf(stderr, "[LOG]: ");
   vfprintf(stderr, Fmt, Args);
   fprintf(stderr, "\n");
   
   va_end(Args);
}