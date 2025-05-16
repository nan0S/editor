internal loaded_image
LoadImageFromMemory(arena *Arena, char *ImageData, u64 Count)
{
 loaded_image Result = {};
 
 stbi_set_flip_vertically_on_load(1);
 int Width, Height;
 int Components;
 int RequestComponents = 4;
 char *Data = Cast(char *)stbi_load_from_memory(Cast(stbi_uc const *)ImageData,
                                                Cast(int)Count,
                                                &Width, &Height, &Components, RequestComponents);
 if (Data)
 {
  u64 TotalSize = Cast(u64)Width * Height * RequestComponents;
  char *Pixels = PushArrayNonZero(Arena, TotalSize, char);
  MemoryCopy(Pixels, Data, TotalSize);
  
  Result.Success = true;
  Result.Width = Width;
  Result.Height = Height;
  Result.Pixels = Pixels;
  
  stbi_image_free(Data);
 }
 
 return Result;
}

internal image_info
LoadImageInfo(string FilePath)
{
 image_info Result = {};
 
 temp_arena Temp = TempArena(0);
 string CFilePath = CStrFromStr(Temp.Arena, FilePath);
 int X, Y, Channels;
 if (stbi_info(CFilePath.Data, &X, &Y, &Channels))
 {
  Assert(X >= 0 && Y >= 0 && Channels >= 0);
  Result.Width = Cast(u32)X;
  Result.Height = Cast(u32)Y;
  Result.Channels = Cast(u32)Channels;
 }
 
 return Result;
}