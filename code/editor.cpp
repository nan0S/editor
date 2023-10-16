#include "editor.h"

#include "editor_memory.cpp"
#include "editor_string.cpp"
#include "editor_file.cpp"
#include "editor_math.cpp"
#include "editor_os.cpp"
#include "editor_input.cpp"
#include "editor_entity.cpp"
#include "editor_project.cpp"
#include "editor_debug.cpp"
#include "editor_draw.cpp"
#include "editor_profiler.cpp"
#include "editor_editor.cpp"

function void
ReportError(char const *Fmt, ...)
{
   va_list ArgList;
   va_start(ArgList, Fmt);
   
   fprintf(stderr, "error: ");
   vfprintf(stderr, Fmt, ArgList);
   fprintf(stderr, "\n");
   
   va_end(ArgList);
}

function void
Log(char const *Fmt, ...)
{
   va_list ArgList;
   va_start(ArgList, Fmt);
   
   fprintf(stderr, "[LOG]: ");
   vfprintf(stderr, Fmt, ArgList);
   fprintf(stderr, "\n");
   
   va_end(ArgList);
}