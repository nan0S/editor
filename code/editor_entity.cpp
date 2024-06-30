//- Name String
#if 0

//- Curve
internal curve
CurveMake(name_string CurveName,
          curve_params CurveParams,
          u64 SelectedControlPointIndex,
          world_position Position,
          rotation_2d Rotation)
{
   curve Result = {};
   Result.Name = CurveName;
   Result.CurveParams = CurveParams;
   Result.SelectedControlPointIndex = SelectedControlPointIndex;
   Result.Position = Position;
   Result.Rotation = Rotation;
   
   return Result;
}

internal curve
CurveCopy(curve Curve)
{
   curve Copy = CurveMake(Curve.Name,
                          Curve.CurveParams,
                          Curve.SelectedControlPointIndex,
                          Curve.Position, Curve.Rotation);
   
   CurveSetControlPoints(&Copy,
                         Curve.NumControlPoints,
                         Curve.ControlPoints,
                         Curve.ControlPointWeights,
                         Curve.CubicBezierPoints);
   
   return Copy;
}

// NOTE(hbr): Trinale-based, no mitter, no-spiky line version.
#if 0
internal line_vertices
CalculateNewLineVertices(u64 NumLinePoints, v2f32 *LinePoints,
                         f32 LineWidth, color LineColor,
                         line_vertices OldLineVertices)
{
   sf::Vertex *Vertices = OldLineVertices.Vertices;
   u64 NumVertices = 10 * NumLinePoints + 100;
   ArrayReserve(Vertices, OldLineVertices.NumVertices, NumVertices);
   
   u64 VertexIndex = 0;
   for (u64 PointIndex = 1;
        PointIndex + 1 < NumLinePoints;
        ++PointIndex)
   {
      v2f32 A = LinePoints[PointIndex - 1];
      v2f32 B = LinePoints[PointIndex + 0];
      v2f32 C = LinePoints[PointIndex + 1];
      
      v2f32 V_Line = B - A;
      Normalize(&V_Line);
      
      v2f32 NV_Line = Rotate90DegreesAntiClockwise(V_Line);
      
      v2f32 V_Succ = C - B;
      Normalize(&V_Succ);
      v2f32 NV_Succ = Rotate90DegreesAntiClockwise(V_Succ);
      
      v2f32 V_Miter = NV_Line + NV_Succ;
      Normalize(&V_Miter);
      
      if (Cross(B - A, C - B) >= 0.0f) // left-turn
      {
         line_intersection Intersection = LineIntersection(A + 0.5f * LineWidth * NV_Line,
                                                           B + 0.5f * LineWidth * NV_Line,
                                                           B + 0.5f * LineWidth * NV_Succ,
                                                           C + 0.5f * LineWidth * NV_Succ);
         
         v2f32 Position1 = {};
         if (Intersection.IsOneIntersection)
         {
            Position1 = Intersection.IntersectionPoint;
         }
         else
         {
            Position1 = B + 0.5f * LineWidth * NV_Line;
         }
         v2f32 Position2 = B - 0.5f * LineWidth * NV_Line;
         v2f32 Position3 = B - 0.5f * LineWidth * NV_Succ;
         
         v2f32 T1 = {};
         v2f32 T2 = {};
         if (PointIndex == 1)
         {
            T1 = A + 0.5f * LineWidth * NV_Line;
            T2 = A - 0.5f * LineWidth * NV_Line;
         }
         else
         {
            // TODO(hbr): Make sure this is ok
            Assert(VertexIndex >= 2);
            T1 = V2F32FromVec(Vertices[VertexIndex - 1].position);
            T2 = V2F32FromVec(Vertices[VertexIndex - 2].position);
         }
         
         Vertices[VertexIndex + 0].position = V2F32ToVector2f(T1);
         Vertices[VertexIndex + 1].position = V2F32ToVector2f(T2);
         Vertices[VertexIndex + 2].position = V2F32ToVector2f(Position1);
         
         Vertices[VertexIndex + 3].position = V2F32ToVector2f(T2);
         Vertices[VertexIndex + 4].position = V2F32ToVector2f(Position2);
         Vertices[VertexIndex + 5].position = V2F32ToVector2f(Position1);
         
         Vertices[VertexIndex + 6].position = V2F32ToVector2f(Position2);
         Vertices[VertexIndex + 7].position = V2F32ToVector2f(Position3);
         Vertices[VertexIndex + 8].position = V2F32ToVector2f(Position1);
         
         Vertices[VertexIndex + 0].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 1].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 2].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 3].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 4].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 5].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 6].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 7].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 8].color = ColorToSFMLColor(LineColor);
         
         VertexIndex += 9;
      }
      else // right-turn
      {
         line_intersection Intersection = LineIntersection(A - 0.5f * LineWidth * NV_Line,
                                                           B - 0.5f * LineWidth * NV_Line,
                                                           B - 0.5f * LineWidth * NV_Succ,
                                                           C - 0.5f * LineWidth * NV_Succ);
         
         v2f32 Position1 = {};
         if (Intersection.IsOneIntersection)
         {
            Position1 = Intersection.IntersectionPoint;
         }
         else
         {
            Position1 = B - 0.5f * LineWidth * NV_Line;
         }
         v2f32 Position2 = B + 0.5f * LineWidth * NV_Line;
         v2f32 Position3 = B + 0.5f * LineWidth * NV_Succ;
         
         v2f32 T1 = {};
         v2f32 T2 = {};
         if (PointIndex == 1)
         {
            T1 = A + 0.5f * LineWidth * NV_Line;
            T2 = A - 0.5f * LineWidth * NV_Line;
         }
         else
         {
            // TODO(hbr): Make sure this is ok
            Assert(VertexIndex >= 2);
            T1 = V2F32FromVec(Vertices[VertexIndex - 1].position);
            T2 = V2F32FromVec(Vertices[VertexIndex - 2].position);
         }
         
         Vertices[VertexIndex + 0].position = V2F32ToVector2f(T1);
         Vertices[VertexIndex + 1].position = V2F32ToVector2f(T2);
         Vertices[VertexIndex + 2].position = V2F32ToVector2f(Position2);
         
         Vertices[VertexIndex + 3].position = V2F32ToVector2f(T2);
         Vertices[VertexIndex + 4].position = V2F32ToVector2f(Position1);
         Vertices[VertexIndex + 5].position = V2F32ToVector2f(Position2);
         
         Vertices[VertexIndex + 6].position = V2F32ToVector2f(Position2);
         Vertices[VertexIndex + 7].position = V2F32ToVector2f(Position1);
         Vertices[VertexIndex + 8].position = V2F32ToVector2f(Position3);
         
         Vertices[VertexIndex + 0].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 1].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 2].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 3].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 4].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 5].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 6].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 7].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 8].color = ColorToSFMLColor(LineColor);
         
         VertexIndex += 9;
      }
   }
   
   NumVertices = VertexIndex;
   
   line_vertices Result = {};
   Result.NumVertices = NumVertices;
   Result.Vertices = Vertices;
   
   return Result;
}
#endif

// NOTE(hbr): Triangle-based, mitter, non-spiky line version.
#if 0
internal line_vertices
CalculateNewLineVertices(u64 NumLinePoints, v2f32 *LinePoints,
                         f32 LineWidth, color LineColor,
                         line_vertices OldLineVertices)
{
   sf::Vertex *Vertices = OldLineVertices.Vertices;
   u64 NumVertices = 10 * NumLinePoints + 100;
   ArrayReserve(Vertices, OldLineVertices.NumVertices, NumVertices);
   
   u64 VertexIndex = 0;
   for (u64 PointIndex = 1;
        PointIndex + 1 < NumLinePoints;
        ++PointIndex)
   {
      v2f32 A = LinePoints[PointIndex - 1];
      v2f32 B = LinePoints[PointIndex + 0];
      v2f32 C = LinePoints[PointIndex + 1];
      
      v2f32 V_Line = B - A;
      Normalize(&V_Line);
      
      v2f32 NV_Line = Rotate90DegreesAntiClockwise(V_Line);
      
      v2f32 V_Succ = C - B;
      Normalize(&V_Succ);
      v2f32 NV_Succ = Rotate90DegreesAntiClockwise(V_Succ);
      
      v2f32 V_Miter = NV_Line + NV_Succ;
      Normalize(&V_Miter);
      
      if (Cross(B - A, C - B) >= 0.0f) // left-turn
      {
         v2f32 Position1 = B + 0.5f * LineWidth / Dot(V_Miter, NV_Line) * V_Miter;
         v2f32 Position2 = B - 0.5f * LineWidth * NV_Line;
         v2f32 Position3 = B - 0.5f * LineWidth * NV_Succ;
         
         v2f32 T1 = {};
         v2f32 T2 = {};
         if (PointIndex == 1)
         {
            T1 = A + 0.5f * LineWidth * NV_Line;
            T2 = A - 0.5f * LineWidth * NV_Line;
         }
         else
         {
            // TODO(hbr): Make sure this is ok
            Assert(VertexIndex >= 2);
            T1 = V2F32FromVec(Vertices[VertexIndex - 1].position);
            T2 = V2F32FromVec(Vertices[VertexIndex - 2].position);
         }
         
         Vertices[VertexIndex + 0].position = V2F32ToVector2f(T1);
         Vertices[VertexIndex + 1].position = V2F32ToVector2f(T2);
         Vertices[VertexIndex + 2].position = V2F32ToVector2f(Position1);
         
         Vertices[VertexIndex + 3].position = V2F32ToVector2f(T2);
         Vertices[VertexIndex + 4].position = V2F32ToVector2f(Position2);
         Vertices[VertexIndex + 5].position = V2F32ToVector2f(Position1);
         
         Vertices[VertexIndex + 6].position = V2F32ToVector2f(Position2);
         Vertices[VertexIndex + 7].position = V2F32ToVector2f(Position3);
         Vertices[VertexIndex + 8].position = V2F32ToVector2f(Position1);
         
         Vertices[VertexIndex + 0].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 1].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 2].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 3].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 4].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 5].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 6].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 7].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 8].color = ColorToSFMLColor(LineColor);
         
         VertexIndex += 9;
      }
      else // right-turn
      {
         v2f32 Position1 = B - 0.5f * LineWidth / Dot(V_Miter, NV_Line) * V_Miter;
         v2f32 Position2 = B + 0.5f * LineWidth * NV_Line;
         v2f32 Position3 = B + 0.5f * LineWidth * NV_Succ;
         
         v2f32 T1 = {};
         v2f32 T2 = {};
         if (PointIndex == 1)
         {
            T1 = A + 0.5f * LineWidth * NV_Line;
            T2 = A - 0.5f * LineWidth * NV_Line;
         }
         else
         {
            // TODO(hbr): Make sure this is ok
            Assert(VertexIndex >= 2);
            T1 = V2F32FromVec(Vertices[VertexIndex - 1].position);
            T2 = V2F32FromVec(Vertices[VertexIndex - 2].position);
         }
         
         Vertices[VertexIndex + 0].position = V2F32ToVector2f(T1);
         Vertices[VertexIndex + 1].position = V2F32ToVector2f(T2);
         Vertices[VertexIndex + 2].position = V2F32ToVector2f(Position2);
         
         Vertices[VertexIndex + 3].position = V2F32ToVector2f(T2);
         Vertices[VertexIndex + 4].position = V2F32ToVector2f(Position1);
         Vertices[VertexIndex + 5].position = V2F32ToVector2f(Position2);
         
         Vertices[VertexIndex + 6].position = V2F32ToVector2f(Position2);
         Vertices[VertexIndex + 7].position = V2F32ToVector2f(Position1);
         Vertices[VertexIndex + 8].position = V2F32ToVector2f(Position3);
         
         Vertices[VertexIndex + 0].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 1].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 2].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 3].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 4].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 5].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 6].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 7].color = ColorToSFMLColor(LineColor);
         Vertices[VertexIndex + 8].color = ColorToSFMLColor(LineColor);
         
         VertexIndex += 9;
      }
   }
   
   NumVertices = VertexIndex;
   
   line_vertices Result = {};
   Result.NumVertices = NumVertices;
   Result.Vertices = Vertices;
   
   return Result;
}
#endif

// NOTE(hbr): Triangle-based, mitter, spike line version
#if 0
internal line_vertices
CalculateNewLineVertices(u64 NumLinePoints, v2f32 *LinePoints,
                         f32 LineWidth, color LineColor,
                         line_vertices OldLineVertices)
{
   if (NumLinePoints == 0)
   {
      return OldLineVertices;
   }
   
   u64 N = NumLinePoints;
   u64 NumVertices = 6 * (N-1);
   sf::Vertex *Vertices = OldLineVertices.Vertices;
   ArrayReserve(Vertices, OldLineVertices.NumVertices, NumVertices);
   
   if (NumLinePoints >= 2)
   {
      temp_arena Temp = TempArena(0);
      defer { EndTemp(Temp); };
      
      v2f32 *LinePointsLooped = PushArrayNonZero(Temp.Arena, NumLinePoints + 3, v2f32);
      for (u64 PointIndex = 0;
           PointIndex < NumLinePoints;
           ++PointIndex)
      {
         LinePointsLooped[PointIndex + 1] = LinePoints[PointIndex];
      }
      
#if 0
      LinePointsLooped[0] = LinePoints[NumLinePoints - 1];
      LinePointsLooped[NumLinePoints + 1] = LinePoints[0];
#else
      LinePointsLooped[0] = LinePoints[0];
      LinePointsLooped[NumLinePoints + 1] = LinePoints[NumLinePoints - 1];
#endif
      //LinePointsLooped[NumLinePoints + 2] = LinePoints[1];
      
      u64 VertexIndex = 0;
      for (u64 LineIndex = 0;
           LineIndex < N-1;
           ++LineIndex)
      {
         v2f32 Points[4] = {};
         for (u64 I = 0; I < 4; ++I) Points[I] = LinePointsLooped[LineIndex + I];
         
         for (u64 TriIndex = 0;
              TriIndex < 6;
              ++TriIndex)
         {
            v2f32 V_Line = Points[2] - Points[1];
            Normalize(&V_Line);
            
            v2f32 NV_Line = Rotate90DegreesAntiClockwise(V_Line);
            
            v2f32 Position = {};
            if (TriIndex == 0 || TriIndex == 1 || TriIndex == 3)
            {
               v2f32 V_Pred = Points[1] - Points[0];
               Normalize(&V_Pred);
               
               v2f32 V_Miter = NV_Line + Rotate90DegreesAntiClockwise(V_Pred);
               Normalize(&V_Miter);
               
               Position = Points[1] + LineWidth * (TriIndex == 1 ? -0.5f : 0.5f) / Dot(V_Miter, NV_Line) * V_Miter;
            }
            else
            {
               v2f32 V_Succ = Points[3]  - Points[2];
               Normalize(&V_Succ);
               
               v2f32 V_Miter = NV_Line + Rotate90DegreesAntiClockwise(V_Succ);
               Normalize(&V_Miter);
               
               Position = Points[2] + LineWidth * (TriIndex == 5 ? 0.5f : -0.5f) / Dot(V_Miter, NV_Line) * V_Miter;
            }
            
            Vertices[VertexIndex].position = V2F32ToVector2f(Position);
            Vertices[VertexIndex].color = ColorToSFMLColor(LineColor);
            VertexIndex += 1;
         }
      }
      Assert(VertexIndex == NumVertices);
   }
   
   line_vertices Result = {};
   Result.NumVertices = NumVertices;
   Result.Vertices = Vertices;
   
   return Result;
}
#endif

//- Image
internal image
ImageMake(name_string Name, world_position Position,
          v2f32 Scale, rotation_2d Rotation,
          s64 SortingLayer, b32 Hidden,
          string FilePath, sf::Texture Texture)
{
   image Result = {};
   Result.Name = Name;
   Result.Position = Position;
   Result.Scale = Scale;
   Result.Rotation = Rotation;
   Result.SortingLayer = SortingLayer;
   Result.Hidden = Hidden;
   Result.FilePath = DuplicateStr(FilePath);
   Result.Texture = Texture;
   
   return Result;
}

internal sf::Texture
LoadTextureFromFile(arena *Arena, string FilePath, error_string *OutError)
{
   sf::Texture Texture;
   
   temp_arena Temp = TempArena(Arena);
   defer { EndTemp(Temp); };
   
   error_string FileReadError = 0;
   file_contents TextureFileContents = ReadEntireFile(Temp.Arena, FilePath, &FileReadError);
   
   if (FileReadError)
   {
      *OutError = StrF(Arena,
                       "failed to load texture from file: %s",
                       FileReadError);
   }
   else
   {
      if (!Texture.loadFromMemory(TextureFileContents.Contents, TextureFileContents.Size))
      {
         *OutError = StrF(Arena,
                          "failed to load texture from file %s",
                          FilePath);
      }
   }
   
   return Texture;
}

internal image
ImageCopy(image Image)
{
   image Result = ImageMake(Image.Name, Image.Position,
                            Image.Scale, Image.Rotation,
                            Image.SortingLayer, Image.Hidden,
                            Image.FilePath, Image.Texture);
   
   return Result;
}

internal entity
CurveEntity(curve Curve)
{
   entity Result = {};
   Result.Type = Entity_Curve;
   Result.Curve = Curve;
   
   return Result;
}

internal entity
ImageEntity(image Image)
{
   entity Result = {};
   Result.Type = Entity_Image;
   Result.Image = Image;
   
   return Result;
}

internal void
EntityDestroy(entity *Entity)
{
   switch (Entity->Type)
   {
      case Entity_Curve: {
         CurveDestroy(&Entity->Curve);
      } break;
      
      case Entity_Image: {
         FreeStr(Image->FilePath);
         Image->Texture.~Texture();
         MemoryZero(Cast(void *)Image, SizeOf(Image));
      } break;
   }
}

#endif