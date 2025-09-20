/* ========================================================================
   Parametric Curve Editor
   Master's Thesis by Hubert Obrzut
   Supervisor: Paweł Woźny
   University of Wrocław
   Faculty of Mathematics and Computer Science
   Institute of Computer Science
   Date: September 2025
   ======================================================================== */

#ifndef EDITOR_STB_H
#define EDITOR_STB_H

//#define STBI_NO_STDIO
#include "third_party/stb/stb_image.h"

struct image_info
{
 u32 Width;
 u32 Height;
 u32 ChannelsUponLoad;
 u32 RealChannels;
 u64 SizeInBytesUponLoad;
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
