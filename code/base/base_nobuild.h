#ifndef BASE_NOBUILD_
#define BASE_NOBUILD_H

struct recompilation_result
{
 b32 TriedToRecompile;
 int RecompilationExitCode;
};
internal recompilation_result RecompileYourselfIfNecessary(int ArgCount, char *Argv[]);

#endif //BASE_NOBUILD_H
