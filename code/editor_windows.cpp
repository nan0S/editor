//- Virtual Memory
internal u64
PageSize(void)
{
   SYSTEM_INFO Info;
   GetSystemInfo(&Info);
   return Info.dwPageSize;
}

function void *
VirtualMemoryReserve(u64 Capacity)
{
   u64 GbSnappedCapacity = Capacity;
   GbSnappedCapacity += Gigabytes(1) - 1;
   GbSnappedCapacity -= GbSnappedCapacity % Gigabytes(1);
   void *Result = VirtualAlloc(0, GbSnappedCapacity, MEM_RESERVE, PAGE_NOACCESS);
   
   return Result;
}

function void
VirtualMemoryRelease(void *Memory, u64 Size)
{
   VirtualFree(Memory, 0, MEM_RELEASE);
}

function void
VirtualMemoryCommit(void *Memory, u64 Size)
{
   u64 PageSnappedSize = Size;
   PageSnappedSize += PageSize() - 1;
   PageSnappedSize -= PageSnappedSize % PageSize();
   VirtualAlloc(Memory, PageSnappedSize, MEM_COMMIT, PAGE_READWRITE);
}

//- Timing
function u64
ReadOSTimerFrequency(void)
{
	LARGE_INTEGER Freq;
	QueryPerformanceFrequency(&Freq);
	return Freq.QuadPart;
}

function u64
ReadOSTimer(void)
{
   LARGE_INTEGER Counter;
   QueryPerformanceCounter(&Counter);
   return Counter.QuadPart;
}

#if 0
//- File Handling
function file_handle
OpenFile(arena *Arena, string FilePath, file_access_flags FileAccessFlags, error_string *OutError)
{
   temp_arena Temp = TempArena(Arena);
   defer { EndTemp(Temp); };
   
   string Path16 = Str16From8(Temp.Arena, FilePath);
   
   // rjf: map to w32 access flags
   DWORD desired_access = 0;
   if(access_flags & FileAccessFlags_Read)  { desired_access |= GENERIC_READ; }
   if(access_flags & FileAccessFlags_Write) { desired_access |= GENERIC_WRITE; }
   
   // rjf: create share mode
   DWORD share_mode = 0;
   
   // rjf: create security attributes
   SECURITY_ATTRIBUTES security_attributes =
   {
      (DWORD)SizeOf(SECURITY_ATTRIBUTES),
      0,
      0,
   };
   
   // rjf: map to w32 creation disposition
   DWORD creation_disposition = 0;
   if(!(access_flags & OS_AccessFlag_CreateNew))
   {
      creation_disposition = OPEN_EXISTING;
   }
   
   // rjf: misc.
   DWORD flags_and_attributes = 0;
   HANDLE template_file = 0;
   
   // rjf: open handle
   HANDLE file = CreateFileW((WCHAR*)Path16.str,
                             desired_access,
                             share_mode,
                             &security_attributes,
                             creation_disposition,
                             flags_and_attributes,
                             template_file);
   
   // rjf: accumulate errors
   if(file != INVALID_HANDLE_VALUE)
   {
      // TODO(hbr): append to errors
   }
   
   return File;
}

function error_string
CloseFile(arena *Arena, file_handle FileHandle)
{
   CloseHandle(FileHandle);
   
   // TODO(hbr): Handle error
   return 0;
}

function file_contents
ReadWholeFile(arena *Arena, file_handle FileHandle, error_string *OutError)
{
   String8 result = {0};
   HANDLE handle = FileHandle;
   LARGE_INTEGER off_li = {0};
   off_li.QuadPart = range.min;
   if(SetFilePointerEx(handle, off_li, 0, FILE_BEGIN))
   {
      U64 bytes_to_read = Dim1U64(range);
      U64 bytes_actually_read = 0;
      result.str = PushArrayNonZeroNoZero(arena, U8, bytes_to_read);
      result.size = 0;
      U8 *ptr = result.str;
      U8 *opl = result.str + bytes_to_read;
      for(;;)
      {
         U64 unread = (U64)(opl - ptr);
         DWORD to_read = (DWORD)(ClampTop(unread, U32Max));
         DWORD did_read = 0;
         if(!ReadFile(handle, ptr, to_read, &did_read, 0))
         {
            break;
         }
         ptr += did_read;
         result.size += did_read;
         if(ptr >= opl)
         {
            break;
         }
      }
   }
   return result;
}

core_function void
OS_FileWrite(Arena *arena, OS_Handle file, U64 off, String8List data, OS_ErrorList *out_errors)
{
   HANDLE handle = (HANDLE)file.u64[0];
   LARGE_INTEGER off_li = {0};
   off_li.QuadPart = off;
   if(handle == 0 || handle == INVALID_HANDLE_VALUE)
   {
      // TODO(rjf): accumulate errors
   }
   else if(SetFilePointerEx(handle, off_li, 0, FILE_BEGIN))
   {
      for(String8Node *node = data.first; node != 0; node = node->next)
      {
         U8 *ptr = node->string.str;
         U8 *opl = ptr + node->string.size;
         for(;;)
         {
            U64 unwritten = (U64)(opl - ptr);
            DWORD to_write = (DWORD)(ClampTop(unwritten, U32Max));
            DWORD did_write = 0;
            if(!WriteFile(handle, ptr, to_write, &did_write, 0))
            {
               goto fail_out;
            }
            ptr += did_write;
            if(ptr >= opl)
            {
               break;
            }
         }
      }
   }
   fail_out:;
}
#endif