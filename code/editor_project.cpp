function saved_project_header
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

function saved_project_entity
SavedProjectEntity(editor Editor, entity *Entity)
{
   saved_project_entity Result = {};
   Result.Type = Entity->Type;
   
   // TODO(hbr): Clean this up
   switch (Entity->Type)
   {
      case Entity_Curve: {
         curve *Curve = &Entity->Curve;
         
         u64 NumControlPoints = Curve->NumControlPoints;
         
         saved_project_curve SavedCurve = SavedProjectCurve(Curve->Name,
                                                            Curve->CurveParams,
                                                            Curve->Position,
                                                            Curve->Rotation,
                                                            NumControlPoints,
                                                            Curve->IsSelected,
                                                            Curve->SelectedControlPointIndex);
         
         Result.Curve = SavedCurve;
      } break;
      
      case Entity_Image: {
         image *Image = &Entity->Image;
         
         b32 IsSelected = (Editor.State.SelectedEntity == Entity);
         string ImagePath = Image->FilePath;
         
         saved_project_image SavedImage = SavedProjectImage(Image->Name, StringSize(ImagePath),
                                                            Image->Position, Image->Scale, Image->Rotation,
                                                            Image->SortingLayer, IsSelected, Image->Hidden);
         
         Result.Image = SavedImage;
      } break;
   }
   
   return Result;
}

function saved_project_curve
SavedProjectCurve(name_string CurveName, curve_params CurveParams,
                  world_position Position, rotation_2d Rotation,
                  u64 NumControlPoints, b32 IsSelected,
                  u64 SelectedControlPointIndex)
{
   saved_project_curve Result  = {};
   Result.CurveName = CurveName;
   Result.CurveParams = CurveParams;
   Result.Position = Position;
   Result.Rotation = Rotation;
   Result.NumControlPoints = NumControlPoints;
   Result.IsSelected = IsSelected;
   Result.SelectedControlPointIndex = SelectedControlPointIndex;
   
   return Result;
}

function saved_project_image
SavedProjectImage(name_string ImageName, u64 ImagePathSize, v2f32 Position,
                  v2f32 Scale,  rotation_2d Rotation, u64 SortingLayer,
                  b32 IsSelected, b32 Hidden)
{
   saved_project_image Result = {};
   Result.ImageName = ImageName;
   Result.ImagePathSize = ImagePathSize;
   Result.Position = Position;
   Result.Scale = Scale;
   Result.Rotation = Rotation;
   Result.SortingLayer = SortingLayer;
   Result.IsSelected = IsSelected;
   Result.Hidden = Hidden;
   
   return Result;
}

// TODO(hbr): Take more things by pointer in general
function error_string
SaveProjectInFile(arena *Arena,
                  editor Editor,
                  string SaveFilePath)
{
   auto Scratch = ScratchArena(Arena);
   defer { ReleaseScratch(Scratch); };
   
   string_list SaveData = {};
   
   //- Save header
   saved_project_header Header = SavedProjectHeader(SAVED_PROJECT_MAGIC_VALUE,
                                                    Editor.State.NumEntities,
                                                    Editor.State.EverIncreasingCurveCounter,
                                                    Editor.State.Camera,
                                                    Editor.Parameters,
                                                    Editor.UI_Config,
                                                    Editor.State.CurveAnimation.AnimationSpeed);
   
   string HeaderData = StringMake(Scratch.Arena, cast(char const *)&Header, sizeof(Header));
   StringListPush(Scratch.Arena, &SaveData, HeaderData);
   
   //- Save entities
   ListIter(Entity, Editor.State.EntitiesHead, entity)
   {
      saved_project_entity SavedEntity = SavedProjectEntity(Editor, Entity);
      
      string SavedEntityData = StringMake(Scratch.Arena, cast(char const *)&SavedEntity, sizeof(SavedEntity));
      StringListPush(Scratch.Arena, &SaveData, SavedEntityData);
      
      switch (Entity->Type)
      {
         case Entity_Curve: {
            curve *Curve = &Entity->Curve;
            
            u64 NumControlPoints = Curve->NumControlPoints;
            
            string ControlPoints =
               StringMake(Scratch.Arena,
                          cast(char const *)Curve->ControlPoints,
                          NumControlPoints * sizeof(Curve->ControlPoints[0]));
            StringListPush(Scratch.Arena, &SaveData, ControlPoints);
            
            string ControlPointWeights =
               StringMake(Scratch.Arena,
                          cast(char const *)Curve->ControlPointWeights,
                          NumControlPoints * sizeof(Curve->ControlPointWeights[0]));
            StringListPush(Scratch.Arena, &SaveData, ControlPointWeights);
            
            string CubicBezierPoints =
               StringMake(Scratch.Arena,
                          cast(char const *)Curve->CubicBezierPoints,
                          3 * NumControlPoints * sizeof(Curve->CubicBezierPoints[0]));
            StringListPush(Scratch.Arena, &SaveData, CubicBezierPoints);
         } break;
         
         case Entity_Image: {
            image *Image = &Entity->Image;
            StringListPush(Scratch.Arena, &SaveData, Image->FilePath);
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
      *Data = cast(u8 *)*Data + Expected;
      *BytesLeft -= Expected;
   }
   
   return Result;
}
#define ExpectTypeInData(Type, Data, BytesLeft) \
cast(Type *)ExpectSizeInData(sizeof(Type), Data, BytesLeft)
#define ExpectArrayInData(Count, Type, Data, BytesLeft) \
cast(Type *)ExpectSizeInData((Count) * sizeof(Type), Data, BytesLeft) 

function load_project_result
LoadProjectFromFile(arena *Arena, pool *EntityPool,
                    arena *DeCasteljauVisualizationArena,
                    arena *DegreeLoweringArena, arena *MovingPointArena,
                    arena *CurveAnimationArena, string ProjectFilePath)
{
   // TODO(hbr): Shorten error messages, maybe even don't return
   load_project_result Result = {};
   error_string Error = 0;
   string_list Warnings = {};
   
   auto Scratch = ScratchArena(Arena);
   defer { ReleaseScratch(Scratch); };
   
   error_string ReadError = 0;
   file_contents Contents = ReadEntireFile(Scratch.Arena, ProjectFilePath, &ReadError);
   if (ReadError == 0)
   {
      void *Data = Contents.Contents;
      u64 BytesLeft = Contents.Size;
      
      saved_project_header *Header = ExpectTypeInData(saved_project_header,
                                                      &Data, &BytesLeft);
      
      Result.EditorParameters = Header->EditorParameters;
      Result.UI_Config = Header->UI_Config;
      
      if (Header)
      {
         if (Header->MagicValue == SAVED_PROJECT_MAGIC_VALUE)
         {
            editor_state State = EditorStateMake(EntityPool,
                                                 Header->CurveCounter,
                                                 Header->Camera,
                                                 DeCasteljauVisualizationArena,
                                                 DegreeLoweringArena,
                                                 MovingPointArena,
                                                 CurveAnimationArena,
                                                 Header->CurveAnimationSpeed);
            defer {
               if (Error) EditorStateDestroy(&State);
               else Result.EditorState = State;
            };
            
            //- Parse entities
            u64 NumEntities = Header->NumEntities;
            for (u64 EntityIndex = 0;
                 EntityIndex < NumEntities;
                 ++EntityIndex)
            {
               saved_project_entity *SavedEntity = ExpectTypeInData(saved_project_entity,
                                                                    &Data, &BytesLeft);
               
               if (SavedEntity)
               {
                  switch (SavedEntity->Type)
                  {
                     case Entity_Curve: {
                        saved_project_curve *SavedCurve = &SavedEntity->Curve;
                        
                        curve Curve = CurveMake(SavedCurve->CurveName,
                                                SavedCurve->CurveParams,
                                                SavedCurve->SelectedControlPointIndex,
                                                SavedCurve->Position,
                                                SavedCurve->Rotation);
                        
                        entity Entity = CurveEntity(Curve);
                        entity *AddEntity = EditorStateAddEntity(&State, &Entity);
                        curve *AddCurve = &AddEntity->Curve;
                        
                        if (SavedCurve->IsSelected)
                        {
                           EditorStateSelectEntity(&State, AddEntity);
                        }
                        
                        u64 NumControlPoints = SavedCurve->NumControlPoints;
                        local_position *ControlPoints = ExpectArrayInData(NumControlPoints,
                                                                          local_position,
                                                                          &Data, &BytesLeft);
                        f32 *ControlPointWeights = ExpectArrayInData(NumControlPoints, f32,
                                                                     &Data, &BytesLeft);
                        local_position *CubicBezierPoints = ExpectArrayInData(3 * NumControlPoints,
                                                                              local_position,
                                                                              &Data, &BytesLeft);
                        
                        if (ControlPoints && ControlPointWeights && CubicBezierPoints)
                        {
                           CurveSetControlPoints(AddCurve, NumControlPoints,
                                                 ControlPoints, ControlPointWeights,
                                                 CubicBezierPoints);
                        }
                        else
                        {
                           Error = StringMakeFormat(Arena,
                                                    "project file %s corrupted (unexpected end of data, expected curve control points data)",
                                                    ProjectFilePath);
                        }
                     } break;
                     
                     case Entity_Image: {
                        saved_project_image *SavedImage = &SavedEntity->Image;
                        
                        u64 ImagePathSize = SavedImage->ImagePathSize;
                        char *ImagePathStr = ExpectArrayInData(ImagePathSize, char,
                                                               &Data, &BytesLeft);
                        
                        if (ImagePathStr)
                        {
                           string ImagePath = StringMake(Scratch.Arena, ImagePathStr, ImagePathSize);
                           
                           string LoadTextureError = 0;
                           sf::Texture LoadedTexture = LoadTextureFromFile(Scratch.Arena,
                                                                           ImagePath,
                                                                           &LoadTextureError);
                           
                           if (LoadTextureError)
                           {
                              string Warning = StringMakeFormat(Arena,
                                                                "failed to load image while loading project from file: %s",
                                                                LoadTextureError);
                              StringListPush(Arena, &Warnings, Warning);
                           }
                           else
                           {
                              image NewImage = ImageMake(SavedImage->ImageName,
                                                         SavedImage->Position,
                                                         SavedImage->Scale,
                                                         SavedImage->Rotation,
                                                         SavedImage->SortingLayer,
                                                         SavedImage->Hidden,
                                                         ImagePath,
                                                         LoadedTexture);
                              
                              entity Entity = ImageEntity(NewImage);
                              entity *AddedEntity = EditorStateAddEntity(&State, &Entity);
                              
                              if (SavedImage->IsSelected)
                              {
                                 EditorStateSelectEntity(&State, AddedEntity);
                              }
                           }
                        }
                        else
                        {
                           string Warning = StringMakeFormat(Arena,
                                                             "project file %s corrupted (unexpected end of data, expected saved image path string)",
                                                             ProjectFilePath);
                           StringListPush(Arena, &Warnings, Warning);
                        }
                     } break;
                  }
               }
               else
               {
                  Error = StringMakeFormat(Arena,
                                           "project file %s corrupted (unexpected end of data, expected saved entity data)",
                                           ProjectFilePath);
               }
               
               if (Error)
               {
                  break;
               }
            }
            
            if (!Error)
            {
               if (BytesLeft > 0)
               {
                  string Warning = StringMakeFormat(Arena,
                                                    "project file %s has unexpected trailing data",
                                                    ProjectFilePath);
                  StringListPush(Arena, &Warnings, Warning);
               }
            }
         }
         else
         {
            Error = StringMakeFormat(Arena,
                                     "project file %s corrupted (magic value doesn't match)",
                                     ProjectFilePath);
         }
      }
      else
      {
         Error = StringMakeFormat(Arena,
                                  "project file %s corrupted (header too short)",
                                  ProjectFilePath);
      }
   }
   else
   {
      Error = StringMakeFormat(Arena,
                               "failed to load project from file: %s",
                               ReadError);
   }
   
   Result.Error = Error;
   Result.Warnings = Warnings;
   
   return Result;
}

