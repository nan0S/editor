#define SAVED_PROJECT_MAGIC_VALUE 0xdeadc0dedeadc0deull
struct saved_project_header
{
   u64 MagicValue;
   
   u64 NumEntities;
   u64 CurveCounter;
   
   camera Camera;
   
   editor_parameters EditorParameters;
   
   ui_config UI_Config;
   
   // TODO(hbr): Should it really belong here?
   f32 CurveAnimationSpeed;
};

struct saved_project_curve
{
   curve_params CurveParams;
   u64 NumControlPoints;
   u64 SelectedControlPointIndex;
};

struct saved_project_image
{
   u64 ImagePathSize;
};

struct saved_project_entity
{
   entity_type Type;
   
   world_position Position;
   v2f32 Scale;
   rotation_2d Rotation;
   name_string Name;
   s64 SortingLayer;
   u64 RenamingFrame;
   entity_flags Flags;
   
   union {
      saved_project_curve Curve;
      saved_project_image Image;
   };
};

internal saved_project_header
SavedProjectHeader(u64 MagicValue, u64 NumEntities,
                   u64 CurveCounter, camera Camera,
                   editor_parameters EditorParameters,
                   ui_config UI_Config, f32 CurveAnimationSpeed)
{
   saved_project_header Result = {};
   Result.MagicValue = MagicValue;
   Result.NumEntities = NumEntities;
   Result.CurveCounter = CurveCounter;
   Result.Camera = Camera;
   Result.EditorParameters = EditorParameters;
   Result.UI_Config = UI_Config;
   Result.CurveAnimationSpeed = CurveAnimationSpeed;
   
   return Result;
}

internal saved_project_curve
SavedProjectCurve(curve_params CurveParams,
                  u64 NumControlPoints,
                  u64 SelectedControlPointIndex)
{
   saved_project_curve Result = {};
   Result.CurveParams = CurveParams;
   Result.NumControlPoints = NumControlPoints;
   Result.SelectedControlPointIndex = SelectedControlPointIndex;
   
   return Result;
}

internal saved_project_image
SavedProjectImage(u64 ImagePathSize)
{
   saved_project_image Result = {};
   Result.ImagePathSize = ImagePathSize;
   
   return Result;
}

internal saved_project_entity
SavedProjectEntity(entity *Entity)
{
   saved_project_entity Result = {};
   Result.Type = Entity->Type;
   Result.Position;
   Result.Scale = Entity->Scale;
   Result.Rotation = Entity->Rotation;
   Result.Name = Entity->Name;
   Result.SortingLayer = Entity->SortingLayer;
   Result.Flags = Entity->Flags;
   
   switch (Entity->Type)
   {
      case Entity_Curve: {
         curve *Curve = &Entity->Curve;
         Result.Curve = SavedProjectCurve(Curve->CurveParams,
                                          Curve->NumControlPoints,
                                          Curve->SelectedControlPointIndex);
      } break;
      
      case Entity_Image: {
         Result.Image = SavedProjectImage(Entity->Image.FilePath.Count);
      } break;
   }
   
   return Result;
}

// TODO(hbr): Take more things by pointer in general
function error_string
SaveProjectInFile(arena *Arena, editor Editor, string SaveFilePath)
{
   temp_arena Temp = TempArena(Arena);
   defer { EndTemp(Temp); };
   
   string_list SaveData = {};
   
   saved_project_header Header = SavedProjectHeader(SAVED_PROJECT_MAGIC_VALUE,
                                                    Editor.State.NumEntities,
                                                    Editor.State.EverIncreasingCurveCounter,
                                                    Editor.State.Camera,
                                                    Editor.Parameters,
                                                    Editor.UI_Config,
                                                    Editor.State.CurveAnimation.AnimationSpeed);
   string HeaderData = Str(Temp.Arena, Cast(char const *)&Header, SizeOf(Header));
   StringListPush(Temp.Arena, &SaveData, HeaderData);
   
   ListIter(Entity, Editor.State.EntitiesHead, entity)
   {
      saved_project_entity Saved = SavedProjectEntity(Entity);
      string SavedData = Str(Temp.Arena, Cast(char const *)&Saved, SizeOf(Saved));
      StringListPush(Temp.Arena, &SaveData, SavedData);
      
      switch (Entity->Type)
      {
         case Entity_Curve: {
            curve *Curve = &Entity->Curve;
            
            u64 NumControlPoints = Curve->NumControlPoints;
            
            string ControlPoints = Str(Temp.Arena,
                                       Cast(char const *)Curve->ControlPoints,
                                       NumControlPoints * SizeOf(Curve->ControlPoints[0]));
            StringListPush(Temp.Arena, &SaveData, ControlPoints);
            
            string ControlPointWeights = Str(Temp.Arena,
                                             Cast(char const *)Curve->ControlPointWeights,
                                             NumControlPoints * SizeOf(Curve->ControlPointWeights[0]));
            StringListPush(Temp.Arena, &SaveData, ControlPointWeights);
            
            string CubicBezierPoints = Str(Temp.Arena,
                                           Cast(char const *)Curve->CubicBezierPoints,
                                           3 * NumControlPoints * SizeOf(Curve->CubicBezierPoints[0]));
            StringListPush(Temp.Arena, &SaveData, CubicBezierPoints);
         } break;
         
         case Entity_Image: {
            StringListPush(Temp.Arena, &SaveData, Entity->Image.FilePath);
         } break;
      }
   }
   
   error_string Error = SaveToFile(Arena, SaveFilePath, SaveData);
   return Error;
}

internal void *
ExpectSizeInData(u64 Expected, void **Data, u64 *BytesLeft)
{
   void *Result = 0;
   if (*BytesLeft >= Expected)
   {
      Result = *Data;
      *Data = Cast(u8 *)*Data + Expected;
      *BytesLeft -= Expected;
   }
   
   return Result;
}
#define ExpectTypeInData(Type, Data, BytesLeft) Cast(Type *)ExpectSizeInData(SizeOf(Type), Data, BytesLeft)
#define ExpectArrayInData(Count, Type, Data, BytesLeft) Cast(Type *)ExpectSizeInData((Count) * SizeOf(Type), Data, BytesLeft)

// TODO(hbr): Error doesn't have to be so specific, maybe just "project file is corrupted or something like that"
function load_project_result
LoadProjectFromFile(arena *Arena,
                    string ProjectFilePath,
                    pool *EntityPool,
                    arena *DeCasteljauVisualizationArena,
                    arena *DegreeLoweringArena, arena *MovingPointArena,
                    arena *CurveAnimationArena)
{
   // TODO(hbr): Shorten error messages, maybe even don't return
   load_project_result Result = {};
   error_string Error = {};
   string_list Warnings = {};
   
   editor_state *EditorState = &Result.EditorState;
   editor_parameters *EditorParameters = &Result.EditorParameters;
   ui_config *UI_Config = &Result.UI_Config;
   
   temp_arena Temp = TempArena(Arena);
   defer { EndTemp(Temp); };
   
   error_string ReadError = {};
   file_contents Contents = ReadEntireFile(Temp.Arena, ProjectFilePath, &ReadError);
   if (!IsError(ReadError))
   {
      void *Data = Contents.Contents;
      u64 BytesLeft = Contents.Size;
      b32 Corrupted = false;
      
      saved_project_header *Header = ExpectTypeInData(saved_project_header, &Data, &BytesLeft);
      if (Header && Header->MagicValue)
      {
         *EditorParameters = Header->EditorParameters;
         *UI_Config = Header->UI_Config;
         *EditorState = CreateEditorState(EntityPool,
                                          Header->CurveCounter,
                                          Header->Camera,
                                          DeCasteljauVisualizationArena,
                                          DegreeLoweringArena,
                                          MovingPointArena,
                                          CurveAnimationArena,
                                          Header->CurveAnimationSpeed);
         
         u64 NumEntities = Header->NumEntities;
         for (u64 EntityIndex = 0;
              EntityIndex < NumEntities;
              ++EntityIndex)
         {
            saved_project_entity *SavedEntity = ExpectTypeInData(saved_project_entity, &Data, &BytesLeft);
            if (SavedEntity)
            {
               entity *Entity = AllocateAndAddEntity(EditorState);
               InitEntity(Entity,
                          SavedEntity->Position, SavedEntity->Scale, SavedEntity->Rotation,
                          SavedEntity->Name,
                          SavedEntity->SortingLayer,
                          SavedEntity->RenamingFrame,
                          SavedEntity->Flags,
                          SavedEntity->Type);
               
               switch (SavedEntity->Type)
               {
                  case Entity_Curve: {
                     saved_project_curve *SavedCurve = &SavedEntity->Curve;
                     u64 NumControlPoints = SavedCurve->NumControlPoints;
                     local_position *ControlPoints = ExpectArrayInData(NumControlPoints, local_position, &Data, &BytesLeft);
                     f32 *ControlPointWeights = ExpectArrayInData(NumControlPoints, f32, &Data, &BytesLeft);
                     local_position *CubicBezierPoints = ExpectArrayInData(3 * NumControlPoints, local_position, &Data, &BytesLeft);
                     
                     if (ControlPoints && ControlPointWeights && CubicBezierPoints)
                     {
                        InitCurve(&Entity->Curve,
                                  SavedCurve->CurveParams, SavedCurve->SelectedControlPointIndex,
                                  NumControlPoints, ControlPoints, ControlPointWeights, CubicBezierPoints);
                     }
                     else
                     {
                        Corrupted = true;
                     }
                  } break;
                  
                  case Entity_Image: {
                     saved_project_image *SavedImage = &SavedEntity->Image;
                     image *Image = &Entity->Image;
                     
                     char *ImagePathStr = ExpectArrayInData(SavedImage->ImagePathSize, char, &Data, &BytesLeft);
                     if (ImagePathStr)
                     {
                        string ImagePath = Str(Temp.Arena, ImagePathStr, SavedImage->ImagePathSize);
                        
                        string LoadTextureError = {};
                        sf::Texture LoadedTexture = LoadTextureFromFile(Temp.Arena, ImagePath, &LoadTextureError);
                        
                        if (!IsError(LoadTextureError))
                        {
                           InitImage(Image, ImagePath, &LoadedTexture);
                        }
                        else
                        {
                           string Warning = StrF(Arena, "image texture load failed: %s", LoadTextureError);
                           StringListPush(Arena, &Warnings, Warning);
                        }
                     }
                     else
                     {
                        Corrupted = true;
                     }
                  } break;
               }
               
               if (SavedEntity->Flags & EntityFlag_Selected)
               {
                  SelectEntity(EditorState, Entity);
               }
            }
            else
            {
               Corrupted = true;
            }
            
            if (Corrupted || IsError(Error))
            {
               break;
            }
         }
      }
      else
      {
         Corrupted = true;
      }
      
      if (Corrupted)
      {
         Error = StrC(Arena, "project file corrupted");
      }
      
      if (!IsError(Error))
      {
         if (BytesLeft > 0)
         {
            string Warning = StrC(Arena, "project file has unexpected trailing data");
            StringListPush(Arena, &Warnings, Warning);
         }
      }
   }
   else
   {
      Error = StrC(Arena, ReadError.Data);
   }
   
   if (IsError(Error))
   {
      DestroyEditorState(EditorState);
   }
   
   Result.Error = Error;
   Result.Warnings = Warnings;
   
   return Result;
}

