//-Virtual Memory
function void *
VirtualMemoryReserve(u64 Capacity)
{
   void *Result = mmap(0, Capacity, PROT_NONE, MAP_ANON|MAP_PRIVATE, -1, 0);
   return Result;
}

function void
VirtualMemoryCommit(void *Memory, u64 Size)
{
   mprotect(Memory, Size, PROT_READ|PROT_WRITE);
}

function void
VirtualMemoryRelease(void *Memory, u64 Size)
{
   munmap(Memory, Size);
}

//- Timing
function u64
ReadOSTimerFrequency(void)
{
   return 1000000;
}

function u64
ReadOSTimer(void)
{
   struct timeval Time;
   
   gettimeofday(&Time, 0);
   
   u64 OSFreq = ReadOSTimerFrequency();
   u64 Result = OSFreq * Time.tv_sec + Time.tv_usec;
   
   return Result;
}

#if 0
//- File Handling
function file_handle
OpenFile(arena *Arena, string FilePath, file_access_flags FileAccessFlags, error_string *OutError)
{
   int Flags = 0;
   if ((FileAccessFlags & FileAccessFlags_Read) && (FileAccessFlags & FileAccessFlags_Write))
      Flags = O_RDWR;
   else if (FileAccessFlags & FileAccessFlags_Read)
      Flags = O_RDONLY;
   else if (FileAccessFlags & FileAccessFlags_Write)
      Flags = O_WRONLY;
   
   int Descriptor = open(FilePath, Flags);
   if (Descriptor == -1)
   {
      *OutError = Str(Arena, strerror(errno));
   }
   
   return Descriptor;
}

function error_string
CloseFile(arena *Arena, file_handle FileHandle)
{
   error_string Result = 0;
   if (close(FileHandle) == -1)
   {
      Result = Str(Arena, strerror(errno));
   }
   
   return Result;
}

// TODO(hbr): Change to generic read
function file_contents
ReadWholeFile(arena *Arena, file_handle FileHandle, error_string *OutError)
{
   file_handle Result = {};
   
   struct stat Stat = {};
   if (fstat(Descriptor, &Stat) == 0)
   {
      off_t ToRead = Stat.st_size;
      void *Contents = ArenaPushSize(Arena, ToRead);
      ssize_t Read = read(Descriptor, Contents, ToRead);
      
      if (ToRead == Read)
      {
         Result.Contents = Contents;
         Result.Size = ToRead;
      }
      else
      {
         if (Read == -1)
         {
            *OutError = Str(Arena, strerror(errno));
         }
         else
         {
            *OutError = StrF(Arena,
                             "Unexpected read length while reading file %s (%lu bytes expected, actual %lu)",
                             FilePath, ToRead, Read);
         }
      }
   }
   else
   {
      *OutError = Str(Arena, strerror(errno));
   }
   
   return Result;
}

function error_string
WriteFile(arena *Arena, file_handle FileHandle, string Content)
{
   error_string Result = 0;
   
   u64 ToWrite = StrLength(Content);
   ssize_t Written = write(FileHandle, Content, ToWrite);
   
   if (Cast(u64)Written != ToWrite)
   {
      if (Written == -1)
      {
         Result = Str(Arena, strerror(errno));
      }
      else
      {
         Result = StrF(Arena, "Unexpected write length (%lu expected, actual %lu)\n",
                       ToWrite, Written);
      }
   }
   
   return Result;
}
#endif