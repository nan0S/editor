struct include_parser
{
 char *At;
 u64 Left;
};
internal void
ParserAdvance(include_parser *Parser, u64 By)
{
 Assert(By <= Parser->Left);
 Parser->At += By;
 Parser->Left -= By;
}

internal void
ParserEatWhitespace(include_parser *Parser)
{
 while (Parser->Left && CharIsWhiteSpace(Parser->At[0]))
 {
  ParserAdvance(Parser, 1);
 }
}

internal b32
ParserStringMatchAndEat(include_parser *Parser, string S)
{
 b32 Match = false;
 if (Parser->Left >= S.Count && MemoryEqual(S.Data, Parser->At, S.Count))
 {
  Match = true;
  ParserAdvance(Parser, S.Count);
 }
 
 return Match;
}

internal b32
ParserCharMatchAndEat(include_parser *Parser, char C)
{
 b32 Result = ParserStringMatchAndEat(Parser, MakeStr(&C, 1));
 return Result;
}

internal recompilation_result
RecompileYourselfIfNecessary(int ArgCount, char *Argv[])
{
 recompilation_result Result = {};
 
 temp_arena Temp = TempArena(0);
 arena *Arena = Temp.Arena;
 
 string InvokePathRel = StrFromCStr(Argv[0]);
 string InvokePathAbs = OS_FullPathFromPath(Arena, InvokePathRel);
 string InvokeDir = PathChopLastPart(InvokePathAbs);
 
 string ExeFullName = PathLastPart(InvokePathAbs);
 string ExeBaseName = StrChopLastDot(ExeFullName);
 
 string CodeDir = InvokeDir;
 CodeDir = PathConcat(Arena, CodeDir, StrLit(".."));
 CodeDir = PathConcat(Arena, CodeDir, StrLit("code"));
 
 string SourceCodeFileNameCand1 = StrF(Arena, "%S.c", ExeBaseName);
 string SourceCodeFileNameCand2 = StrF(Arena, "%S.cpp", ExeBaseName);
 
 string SourceCodePath1 = PathConcat(Arena, CodeDir, SourceCodeFileNameCand1);
 string SourceCodePath2 = PathConcat(Arena, CodeDir, SourceCodeFileNameCand2);
 
 b32 Cand1Exists = OS_FileExists(SourceCodePath1);
 b32 Cand2Exists = OS_FileExists(SourceCodePath2);
 
 string SourceCodePath = {};
 if (Cand1Exists && !Cand2Exists)
 {
  SourceCodePath = SourceCodePath1;
 }
 else if (!Cand1Exists && Cand2Exists)
 {
  SourceCodePath = SourceCodePath2;
 }
 else if (!Cand1Exists && !Cand2Exists)
 {
  // NOTE(hbr): Don't log this, this could happen normally when just recompiled instance runs
 }
 else if (Cand1Exists && Cand2Exists)
 {
  OS_PrintErrorF("both %S and %S files exist, don't know which to choose\n", SourceCodePath1, SourceCodePath2);
 }
 
 if (SourceCodePath.Count)
 {
  string_list AllFilesInvolved = {};
  
  string_list Queue = {};
  string_list_node *Node = PushStruct(Arena, string_list_node);
  Node->Str = OS_FullPathFromPath(Arena, SourceCodePath);
  QueuePush(Queue.Head, Queue.Tail, Node);
  
  while (Queue.Head)
  {
   //- find all includes
   string_list_node *Current = Queue.Head;
   QueuePop(Queue.Head, Queue.Tail);
   
   string Dependency = Current->Str;
   StrListPush(Arena, &AllFilesInvolved, Dependency);
   string DependencyDir = PathChopLastPart(Dependency);
   
   string FileContents = OS_ReadEntireFile(Arena, Dependency);
   
   include_parser _Parser = {};
   include_parser *Parser = &_Parser;
   Parser->At = FileContents.Data;
   Parser->Left = FileContents.Count;
   
   while (Parser->Left)
   {
    if (ParserCharMatchAndEat(Parser, '#'))
    {
     ParserEatWhitespace(Parser);
     if (ParserStringMatchAndEat(Parser, StrLit("include")))
     {
      ParserEatWhitespace(Parser);
      if (ParserCharMatchAndEat(Parser, '"'))
      {
       char *SaveAt = Parser->At;
       while (Parser->Left && Parser->At[0] != '"')
       {
        ParserAdvance(Parser, 1);
       }
       u64 Length = Parser->At - SaveAt;
       if (ParserCharMatchAndEat(Parser, '"'))
       {
        string DependencyFile = MakeStr(SaveAt, Length);
        string DependencyPath = PathConcat(Arena, DependencyDir, DependencyFile);
        
        // NOTE(hbr): make sure we do not recurse
        b32 AlreadyExists = false;
        ListIter(Node, AllFilesInvolved.Head, string_list_node)
        {
         string Current = Node->Str;
         if (StrMatch(Current, DependencyPath, false))
         {
          AlreadyExists = true;
          break;
         }
        }
        
        if (!AlreadyExists)
        {
         string_list_node *Node = PushStruct(Arena, string_list_node);
         Node->Str = OS_FullPathFromPath(Arena, DependencyPath);
         QueuePush(Queue.Head, Queue.Tail, Node);
        }
       }
      }
     }
    }
    else
    {
     ParserAdvance(Parser, 1);
    }
   }
  }
  
  //- maybe recompile
  u64 LatestModifyTime = 0;
  ListIter(DependencyFile, AllFilesInvolved.Head, string_list_node)
  {
   file_attrs Attrs = OS_FileAttributes(DependencyFile->Str);
   LatestModifyTime = Max(LatestModifyTime, Attrs.ModifyTime);
  }
  file_attrs ExeAttrs = OS_FileAttributes(InvokePathAbs);
  if (ExeAttrs.ModifyTime < LatestModifyTime)
  {
   Result.TriedToRecompile = true;
   OS_PrintF("[recompiling myself]\n");
   
   // NOTE(hbr): Always debug and never debug info to be fast
   compiler_setup Setup = MakeCompilerSetup(Compiler_Default, true, false, false);
   compilation_target Target = MakeTarget(Exe, SourceCodePath, CompilationFlag_TemporaryTarget);
   
   compilation_command BuildCompile = ComputeCompilationCommand(Setup, Target);
   os_process_handle BuildCompileProcess = OS_ProcessLaunch(BuildCompile.Cmd);
   int SelfRecompilationExitCode = OS_ProcessWait(BuildCompileProcess);
   Result.RecompilationExitCode = SelfRecompilationExitCode;
   
   if (SelfRecompilationExitCode == 0)
   {
    //- run this program again, this time with up to date binary
    string_list InvokeBuild = {};
    // NOTE(hbr): I have to add that stupid dot on Linux
    StrListPushF(Arena, &InvokeBuild, "./%S", BuildCompile.OutputTarget);
    for (int ArgIndex = 1;
         ArgIndex < ArgCount;
         ++ArgIndex)
    {
     string Arg = StrFromCStr(Argv[ArgIndex]);
     StrListPush(Arena, &InvokeBuild, Arg);
    }
    
    os_process_handle BuildProcess = OS_ProcessLaunch(InvokeBuild);
    int BuildProcessExitCode = OS_ProcessWait(BuildProcess);
    Result.RecompilationExitCode = BuildProcessExitCode;
   }
  }
 }
 
 EndTemp(Temp);
 
 return Result;
}