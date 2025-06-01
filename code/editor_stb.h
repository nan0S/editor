#ifndef EDITOR_STB_H
#define EDITOR_STB_H

//#define STBI_NO_STDIO
#include "third_party/stb/stb_image.h"

struct image_info
{
 u32 Width;
 u32 Height;
 u32 Channels;
 u64 SizeInBytes;
};

struct loaded_image
{
 b32 Success;
 image_info Info;
 char *Pixels;
};

internal loaded_image LoadImageFromMemory(arena *Arena, char *ImageData, u64 Count);
internal image_info LoadImageInfo(string FilePath);

#endif //EDITOR_STB_H
