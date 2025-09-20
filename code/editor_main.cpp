/* ========================================================================
   Parametric Curve Editor
   Master's Thesis by Hubert Obrzut
   Supervisor: Paweł Woźny
   University of Wrocław
   Faculty of Mathematics and Computer Science
   Institute of Computer Science
   Date: September 2025
   ======================================================================== */

#if OS_WINDOWS

int WinMain(HINSTANCE Instance,
            HINSTANCE PrevInstance,
            LPSTR     lpCmdLine,
            int       nShowCmd)
{
 MarkUnused(Instance);
 MarkUnused(PrevInstance);
 MarkUnused(lpCmdLine);
 MarkUnused(nShowCmd);
 EntryPoint(__argc, __argv);
 return 0;
}

#else

int main(int ArgCount, char **Argv)
{
 EntryPoint(ArgCount, Argv);
 return 0;
}

#endif