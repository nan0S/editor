#ifndef BASE_RANDOM_H
#define BASE_RANDOM_H

struct random_series
{
 u32 State;
};

#if 0
// NOTE(hbr): If ever widen it to 4x lanes, then these are good seeds
uint32 E0 = 78953890,
uint32 E1 = 235498,
uint32 E2 = 893456,
uint32 E3 = 93453080
#endif

inline internal random_series
RandomSeed(u32 Seed = 78953890)
{
 random_series Result = {};
 Result.State = Seed;
 
 return Result;
}

inline internal u32
RandomNextU32(random_series *Series)
{
 u32 Result = Series->State;
 Result ^= (Result << 13);
 Result ^= (Result >> 17);
 Result ^= (Result << 5);
 Series->State = Result;
 
 return Result;
}

inline internal f32
RandomUnilateral(random_series *Series)
{
 f32 Div = 1.0f / U32_MAX;
 u32 U = RandomNextU32(Series);
 f32 Result = U * Div;
 
 return Result;
}

inline internal f32
RandomBetween(random_series *Series, f32 A, f32 B)
{
 f32 T = RandomUnilateral(Series);
 f32 Result = Lerp(A, B, T);
 
 return Result;
}

inline internal u32
RandomBetween(random_series *Series, u32 Incl, u32 Excl)
{
 u32 U = RandomNextU32(Series);
 u32 Result = Incl + (U % (Excl - Incl));
 
 return Result;
}

inline internal f32
RandomFrom(random_series *Series, f32 *From, u32 Count)
{
 Assert(Count > 0);
 u32 Index = RandomBetween(Series, 0, Count);
 f32 Result = From[Index];
 
 return Result;
}

#endif //BASE_RANDOM_H
