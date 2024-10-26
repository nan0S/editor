#ifndef SFML_EDITOR_H
#define SFML_EDITOR_H

struct sfml_platform_input
{
   u64 EventCount;
#define SFML_PLATFORM_INPUT_MAX_EVENT_COUNT 128
   platform_event Events[SFML_PLATFORM_INPUT_MAX_EVENT_COUNT];
};

#endif //SFML_EDITOR_H
