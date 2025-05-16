#ifndef EDITOR_STB_H
#define EDITOR_STB_H

//#define STBI_NO_STDIO
#include "third_party/stb/stb_image.h"

struct loaded_image
{
 b32 Success;
 u32 Width;
 u32 Height;
 char *Pixels;
};

struct image_info
{
 u32 Width;
 u32 Height;
 u32 Channels;
};

internal loaded_image LoadImageFromFile(arena *Arena, string FilePath);
internal loaded_image LoadImageFromMemory(arena *Arena, char *ImageData, u64 Count);
internal image_info LoadImageInfo(string FilePath);

#endif //EDITOR_STB_H
