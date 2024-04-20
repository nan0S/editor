function file_contents
ReadEntireFile(arena *Arena, string FilePath, error_string *OutError)
{
   Assert(OutError);
   
   file_contents Result = {};
   b32 IsError = false;
   
   FILE *File = fopen(FilePath, "rb");
   if (File)
   {
      fseek(File, 0L, SEEK_END);
      long FileSize = ftell(File);
      rewind(File);
      
      void *Contents = ArenaPushSize(Arena, FileSize);
      size_t Read = fread(Contents, FileSize, 1, File);
      
      if (Read*FileSize == FileSize)
      {
         Result.Contents = Contents;
         Result.Size = Cast(u64)FileSize;
      }
      else
      {
         *OutError = StringMakeFormat(Arena,
                                      "error while reading file %s",
                                      FilePath);
         IsError = true;
      }
      
      if (fclose(File) != 0)
      {
         if (!IsError)
         {
            *OutError = StringMakeFormat(Arena,
                                         "error while closing file %s",
                                         FilePath);
            IsError = true;
         }
      }
   }
   else
   {
      *OutError = StringMakeFormat(Arena,
                                   "failed to open file %s (%s)",
                                   FilePath,
                                   strerror(errno));
      IsError = true;
   }
   
   return Result;
}

function error_string
SaveToFile(arena *Arena, string FilePath, string_list Data)
{
   error_string Error = 0;
   
   FILE *File = fopen(FilePath, "wb");
   if (File)
   {
      ListIter(Node, Data.Head, string_list_node)
      {
         u64 ToWrite = StringSize(Node->String);
         size_t Written = fwrite(Node->String, ToWrite, 1, File);
         
         if (Written*ToWrite != ToWrite)
         {
            Error = StringMakeFormat(Arena,
                                     "Error while writing to file %s",
                                     FilePath);
            break;
         }
      }
      
      if (fclose(File) != 0)
      {
         if (!Error)
         {
            Error = StringMakeFormat(Arena,
                                     "error while closing file %s",
                                     FilePath);
         }
      }
   }
   else
   {
      Error = StringMakeFormat(Arena,
                               "failed to open file %s (%s)",
                               FilePath,
                               strerror(errno));
   }
   
   return Error;
}