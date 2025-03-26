#ifndef BASE_NOBUILD_
#define BASE_NOBUILD_H

struct recompilation_result
{
 b32 TriedToRecompile;
 exit_code_int RecompilationExitCode;
};
internal recompilation_result RecompileYourselfIfNecessary(int ArgCount, char *Argv[]);

#endif //BASE_NOBUILD_H
