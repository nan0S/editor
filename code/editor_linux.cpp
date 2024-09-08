internal void *
OS_AllocVirtualMemory(u64 Capacity, b32 Commit)
{
   int Prot = (Commit ? PROT_READ | PROT_WRITE : PROT_NONE);
   void *Result = mmap(0, Capacity, Prot, MAP_ANON|MAP_PRIVATE, -1, 0);
   return Result;
}

internal void
OS_CommitVirtualMemory(void *Memory, u64 Size)
{
   mprotect(Memory, Size, PROT_READ|PROT_WRITE);
}

internal void
OS_DeallocVirtualMemory(void *Memory, u64 Size)
{
   munmap(Memory, Size);
}

internal u64
ReadOSTimerFrequency(void)
{
   return 1000000;
}

internal u64
ReadOSTimer(void)
{
   struct timeval Time;
   
   gettimeofday(&Time, 0);
   
   u64 OSFreq = ReadOSTimerFrequency();
   u64 Result = OSFreq * Time.tv_sec + Time.tv_usec;
   
   return Result;
}

internal b32
OS_IterDir(arena *Arena, string Path, dir_iter *Iter, dir_entry *OutEntry)
{
   b32 Found = false;
   
   if (!Iter->NotFirstTime)
   {
      temp_arena Temp = TempArena(Arena);
      string CPath = CStrFromStr(Temp.Arena, Path);
      Iter->Dir = opendir(CPath.Data);
      EndTemp(Temp);
   }
   
   if (Iter->Dir)
   {
      b32 Looking = true;
      while (Looking)
      {
         struct dirent *Entry = readdir(Iter->Dir);
         if (Entry)
         {
            OutEntry->FileName = CStrCopy(Arena, Entry->d_name);
            // TODO(hbr): Use d_reclen to extaract other information about the file
            OutEntry->Attrs.Dir = (Entry->d_type == DT_DIR);
            Found = true;
            Looking = false;
         }
         else
         {
            Looking = false;
         }
      }
   }
   
   if (!Found)
   {
      closedir(Iter->Dir);
   }
   
   return Found;
}
