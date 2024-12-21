#ifndef PLATFORM_SHARED_H
#define PLATFORM_SHARED_H

struct shared_platform_input
{
 u32 EventCount;
#define SHARED_PLATFORM_MAX_EVENT_COUNT 128
 platform_event Events[SHARED_PLATFORM_MAX_EVENT_COUNT];
};

#endif //PLATFORM_SHARED_H