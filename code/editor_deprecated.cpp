/* ========================================================================
   Parametric Curve Editor
   Master's Thesis by Hubert Obrzut
   Supervisor: Paweł Woźny
   University of Wrocław
   Faculty of Mathematics and Computer Science
   Institute of Computer Science
   Date: September 2025
   ======================================================================== */

// NOTE(hbr): O(n^2) time, O(n) memory versions for reference
internal v2
BezierCurveEvaluate(f32 T, v2 *P, u32 N)
{
 temp_arena Temp = TempArena(0);
 
 v2 *E = PushArrayNonZero(Temp.Arena, N, v2);
 MemoryCopy(E, P, N * SizeOf(E[0]));
 
 for (u32 I = 1; I < N; ++I)
 {
  for (u32 J = 0; J < N-I; ++J)
  {
   E[J] = (1-T)*E[J] + T*E[J+1];
  }
 }
 
 v2 Result = E[0];
 EndTemp(Temp);
 
 return Result;
}

internal v2
BezierCurveEvaluateRational(f32 T, v2 *P, f32 *W, u32 N)
{
 temp_arena Temp = TempArena(0);
 
 v2 *EP = PushArrayNonZero(Temp.Arena, N, v2);
 f32 *EW = PushArrayNonZero(Temp.Arena, N, f32);
 MemoryCopy(EP, P, N * SizeOf(EP[0]));
 MemoryCopy(EW, W, N * SizeOf(EW[0]));
 
 for (u32 I = 1; I < N; ++I)
 {
  for (u32 J = 0; J < N-I; ++J)
  {
   f32 EWJ = (1-T)*EW[J] + T*EW[J+1];
   
   f32 WL = EW[J] / EWJ;
   f32 WR = EW[J+1] / EWJ;
   
   EP[J] = (1-T)*WL*EP[J] + T*WR*EP[J+1];
   EW[J] = EWJ;
  }
 }
 
 v2 Result = EP[0];
 EndTemp(Temp);
 
 return Result;
}
