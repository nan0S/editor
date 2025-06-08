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