#ifndef EDITOR_FILE_H
#define EDITOR_FILE_H

struct file_contents
{
   void *Contents;
   u64 Size;
};

function file_contents ReadEntireFile(arena *Arena, string FilePath, error_string *OutError);
function error_string SaveToFile(arena *Arena, string FilePath, string_list Data);

#endif //EDITOR_FILE_H
