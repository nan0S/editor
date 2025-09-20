/* ========================================================================
   Parametric Curve Editor
   Master's Thesis by Hubert Obrzut
   Supervisor: Paweł Woźny
   University of Wrocław
   Faculty of Mathematics and Computer Science
   Institute of Computer Science
   Date: September 2025
   ======================================================================== */

#ifndef BASE_NOBUILD_H
#define BASE_NOBUILD_H

struct recompilation_result
{
 b32 TriedToRecompile;
 exit_code_int RecompilationExitCode;
};
internal recompilation_result RecompileYourselfIfNecessary(int ArgCount, char *Argv[]);

#endif //BASE_NOBUILD_H
