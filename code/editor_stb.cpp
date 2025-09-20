/* ========================================================================
   Parametric Curve Editor
   Master's Thesis by Hubert Obrzut
   Supervisor: Paweł Woźny
   University of Wrocław
   Faculty of Mathematics and Computer Science
   Institute of Computer Science
   Date: September 2025
   ======================================================================== */

internal image_info
MakeImageInfo(u32 Width, u32 Height, u32 Channels)
{
 image_info Info = {};
 Info.Width = Width;
 Info.Height = Height;
 Info.ChannelsUponLoad = 4;
 Info.RealChannels = Channels;
 Info.SizeInBytesUponLoad = Cast(u64)Width * Height * Info.ChannelsUponLoad;
 return Info;
}

internal loaded_image
LoadImageFromMemory(arena *Arena, char *ImageData, u64 Count)
{
 loaded_image Result = {};
 
 stbi_set_flip_vertically_on_load(1);
 int Width, Height;
 int Components;
 int RequestChannels = 4;
 char *Data = Cast(char *)stbi_load_from_memory(Cast(stbi_uc const *)ImageData,
                                                Cast(int)Count,
                                                &Width, &Height, &Components, RequestChannels);
 if (Data)
 {
  u64 TotalSize = Cast(u64)Width * Height * RequestChannels;
  char *Pixels = PushArrayNonZero(Arena, TotalSize, char);
  MemoryCopy(Pixels, Data, TotalSize);
  
  Result.Success = true;
  Result.Info = MakeImageInfo(Width, Height, RequestChannels);
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
  Result = MakeImageInfo(X, Y, Channels);
 }
 
 return Result;
}