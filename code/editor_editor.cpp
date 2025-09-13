internal void
InitLeftClickState(editor_left_click_state *LeftClick, arena_store *ArenaStore)
{
 LeftClick->OriginalVerticesArena = AllocArenaFromStore(ArenaStore, Megabytes(128));
}

internal void
InitFrameStats(frame_stats *FrameStats)
{
 FrameStats->Calculation.MinFrameTime = +F32_INF;
 FrameStats->Calculation.MaxFrameTime = -F32_INF;
}

internal void
InitMergingCurvesState(merging_curves_state *State,
                       entity_store *EntityStore)
{
 State->MergeEntity = AllocEntity(EntityStore, true);
}

internal void
InitVisualProfiler(visual_profiler_state *VisualProfiler, profiler *Profiler)
{
 VisualProfiler->ReferenceMs = VisualProfiler->DefaultReferenceMs = 1000.0f / 120;
 VisualProfiler->Profiler = Profiler;
}

internal void
InitAnimatingCurvesState(animating_curves_state *State,
                         entity_store *EntityStore)
{
 State->ExtractEntity = AllocEntity(EntityStore, true);
}

internal string
GetBaseProjectTitle(editor *Editor)
{
 string BaseTitle = {};
 if (Editor->IsProjectFileBacked)
 {  
  BaseTitle = StrAfterLastSlash(Editor->ProjectFilePath);
 }
 else
 {
  BaseTitle = StrLit("Untitled");
 }
 return BaseTitle;
}

internal void
UpdateWindowTitle(editor *Editor)
{
 temp_arena Temp = TempArena(0);
 string BaseTitle = GetBaseProjectTitle(Editor);
 string ModifiedIndicator = {};
 if (Editor->ProjectModified)
 {
  ModifiedIndicator = StrLit("*");
 }
 string WindowTitle = StrF(Temp.Arena,
                           "%S%S - %S",
                           BaseTitle,
                           ModifiedIndicator,
                           EditorAppName);
 Platform.SetWindowTitle(WindowTitle);
 EndTemp(Temp);
}

internal void
MarkProjectAsModified(editor *Editor)
{
 if (!Editor->ProjectModified)
 {
  Editor->ProjectModified = true;
  UpdateWindowTitle(Editor);
 }
}

internal void
MarkProjectAsUpToDate(editor *Editor)
{
 if (Editor->ProjectModified)
 {
  Editor->ProjectModified = false;
  UpdateWindowTitle(Editor);
 }
}

internal void
ProcessImageLoadingTasks(editor *Editor)
{
 image_loading_store *ImageLoadingStore = Editor->ImageLoadingStore;
 entity_store *EntityStore = Editor->EntityStore;
 string_store *StrStore = Editor->StrStore;
 
 ListIter(ImageLoading,
          ImageLoadingStore->Head,
          image_loading_task)
 {
  if (ImageLoading->State == Image_Loading)
  {
   // NOTE(hbr): nothing to do
  }
  else
  {
   image_instantiation_spec InstantiationSpec = ImageLoading->ImageInstantiationSpec;
   if (ImageLoading->State == Image_Loaded)
   {
    entity *Entity = 0;
    switch (InstantiationSpec.Type)
    {
     case ImageInstantiationSpec_EntityProvided: {
      Entity = EntityFromHandle(InstantiationSpec.ImageEntity);
      if (Entity)
      {
       image *Image = &Entity->Image;
       InitEntityImagePart(Image,
                           ImageLoading->ImageInfo.Width,
                           ImageLoading->ImageInfo.Height,
                           ImageLoading->ImageFilePath,
                           ImageLoading->LoadingTexture);
      }
     }break;
     
     case ImageInstantiationSpec_AtP: {
      Entity = AddEntity(Editor);
      InitEntityAsImage(Entity,
                        InstantiationSpec.InstantiateImageAtP,
                        ImageLoading->ImageInfo.Width,
                        ImageLoading->ImageInfo.Height,
                        ImageLoading->ImageFilePath,
                        ImageLoading->LoadingTexture);
     }break;
    }
    if (Entity)
    {
     SelectEntity(Editor, Entity);
    }
   }
   else
   {
    Assert(ImageLoading->State == Image_Failed);
    DeallocTextureHandle(EntityStore, ImageLoading->LoadingTexture);
    
    if (InstantiationSpec.Type == ImageInstantiationSpec_EntityProvided)
    {
     entity *Entity = EntityFromHandle(InstantiationSpec.ImageEntity);
     if (Entity)
     {
      DeallocEntity(EntityStore, Entity);
     }
    }
    AddNotificationF(Editor, Notification_Error,
                     "failed to load image from \"%S\"",
                     ImageLoading->ImageFilePath);
   }
   
   FinishAsyncImageLoadingTask(ImageLoadingStore, ImageLoading);
  }
 }
}

struct load_image_work
{
 thread_task_memory_store *Store;
 thread_task_memory *TaskMemory;
 renderer_transfer_op *TextureOp;
 string ImagePath;
 image_loading_task *ImageLoading;
};

internal void
LoadImageWork(void *UserData)
{
 load_image_work *Work = Cast(load_image_work *)UserData;
 
 thread_task_memory_store *Store = Work->Store;
 thread_task_memory *Task = Work->TaskMemory;
 renderer_transfer_op *TextureOp = Work->TextureOp;
 string ImagePath = Work->ImagePath;
 image_loading_task *ImageLoading = Work->ImageLoading;
 arena *Arena = Task->Arena;
 
 string ImageData = Platform.ReadEntireFile(Arena, Work->ImagePath);
 loaded_image LoadedImage = LoadImageFromMemory(Arena, ImageData.Data, ImageData.Count);
 
 image_loading_state AsyncTaskState;
 renderer_transfer_op_state OpState;
 if (LoadedImage.Success)
 {
  // TODO(hbr): Don't do memory copy, just read into that memory directly
  MemoryCopy(TextureOp->Pixels, LoadedImage.Pixels, LoadedImage.Info.SizeInBytesUponLoad);
  OpState = RendererOp_ReadyToTransfer;
  AsyncTaskState = Image_Loaded;
 }
 else
 {
  OpState = RendererOp_Empty;
  AsyncTaskState = Image_Failed;
 }
 
 CompilerWriteBarrier;
 TextureOp->State = OpState;
 ImageLoading->State = AsyncTaskState;
 
 EndThreadTaskMemory(Store, Task);
}

internal void
TryLoadImage(editor *Editor, string FilePath, image_instantiation_spec Spec)
{
 work_queue *WorkQueue = Editor->LowPriorityQueue;
 entity_store *EntityStore = Editor->EntityStore;
 thread_task_memory_store *ThreadTaskMemoryStore = Editor->ThreadTaskMemoryStore;
 image_loading_store *ImageLoadingStore = Editor->ImageLoadingStore;
 
 image_info ImageInfo = LoadImageInfo(FilePath);
 render_texture_handle TextureHandle = AllocTextureHandle(EntityStore);
 renderer_transfer_op *TextureOp = PushTextureTransfer(Editor->RendererQueue, ImageInfo.Width, ImageInfo.Height, ImageInfo.SizeInBytesUponLoad, TextureHandle);
 thread_task_memory *TaskMemory = BeginThreadTaskMemory(ThreadTaskMemoryStore);
 
 image_loading_task *ImageLoading = BeginAsyncImageLoadingTask(ImageLoadingStore);
 ImageLoading->ImageInstantiationSpec = Spec;
 ImageLoading->ImageInfo = ImageInfo;
 ImageLoading->ImageFilePath = StrCopy(ImageLoading->Arena, FilePath);
 ImageLoading->LoadingTexture = TextureHandle;
 
 load_image_work *Work = PushStruct(TaskMemory->Arena, load_image_work);
 Work->Store = ThreadTaskMemoryStore;
 Work->TaskMemory = TaskMemory;
 Work->TextureOp = TextureOp;
 Work->ImagePath = StrCopy(TaskMemory->Arena, FilePath);
 Work->ImageLoading = ImageLoading;
 
 Platform.WorkQueueAddEntry(WorkQueue, LoadImageWork, Work);
}

internal void
TryLoadImages(editor *Editor, u32 FileCount, string *FilePaths, v2 AtP)
{
 ForEachIndex(FileIndex, FileCount)
 {
  string FilePath = FilePaths[FileIndex];
  image_instantiation_spec Spec = MakeImageInstantiationSpec_AtP(AtP);
  TryLoadImage(Editor, FilePath, Spec);
 }
}

internal arena_store *
AllocArenaStore(void)
{
 arena *Arena = AllocArena(Megabytes(1));
 arena_store *ArenaStore = PushStruct(Arena, arena_store);
 ArenaStore->Arena = Arena;
 return ArenaStore;
}

internal void
DeallocArenaStoreAndAllArenas(arena_store *ArenaStore)
{
 ListIter(Node, ArenaStore->Head, arena_node)
 {
  if (!Node->Deallocated)
  {
   DeallocArena(Node->Arena);
   Node->Deallocated = true;
  }
 }
 DeallocArena(ArenaStore->Arena);
}

internal arena *
AllocArenaFromStore(arena_store *ArenaStore, u64 ReserveButNotCommit)
{
 arena_node *Node = PushStruct(ArenaStore->Arena, arena_node);
 arena *Arena = AllocArena(ReserveButNotCommit);
 Node->Arena = Arena;
 QueuePush(ArenaStore->Head, ArenaStore->Tail, Node);
 return Arena;
}

internal void
DeallocArenaFromStore(arena_store *ArenaStore, arena *Arena)
{
 b32 Deallocated = false;
 ListIter(Node, ArenaStore->Head, arena_node)
 {
  if (Node->Arena == Arena)
  {
   DeallocArena(Arena);
   Node->Deallocated = true;
   Deallocated = true;
   break;
  }
 }
 Assert(Deallocated);
}

internal void
InitProjectChangeRequestState(arena_store *ArenaStore,
                              project_change_request_state *State)
{
 State->Arena = AllocArenaFromStore(ArenaStore, Megabytes(1));
}

internal void
AllocEditorResources(editor *Editor)
{
 editor_persistent_state Persistent = Editor->PersistentState;
 editor_memory *Memory = Persistent.Memory;
 
 if (Editor->ArenaStore)
 {
  DeallocArenaStoreAndAllArenas(Editor->ArenaStore);
  StructZero(Editor);
 }
 
 arena_store *ArenaStore = AllocArenaStore();
 Editor->PersistentState = Persistent;
 Editor->ArenaStore = ArenaStore;
 Editor->Arena = AllocArenaFromStore(ArenaStore, Gigabytes(64));
 Editor->LowPriorityQueue = Memory->LowPriorityQueue;
 Editor->HighPriorityQueue = Memory->HighPriorityQueue;
 Editor->RendererQueue = Memory->RendererQueue;
 Editor->StrStore = AllocStringStore(ArenaStore);
 Editor->CurvePointsStore = AllocCurvePointsStore(ArenaStore);
 Editor->EntityStore = AllocEntityStore(ArenaStore, Memory->MaxTextureCount, Memory->MaxBufferCount);
 Editor->ThreadTaskMemoryStore = AllocThreadTaskMemoryStore(ArenaStore);
 Editor->ImageLoadingStore = AllocImageLoadingStore(ArenaStore);
 Editor->ProjectFilePathArena = AllocArenaFromStore(ArenaStore, Megabytes(1));
 Editor->NotificationsArena = AllocArenaFromStore(ArenaStore, Megabytes(1));
 
 InitEditorCtx(Editor->ArenaStore,
               Editor->RendererQueue,
               Editor->EntityStore,
               Editor->ThreadTaskMemoryStore,
               Editor->ImageLoadingStore,
               Editor->StrStore,
               Editor->CurvePointsStore,
               Editor->LowPriorityQueue,
               Editor->HighPriorityQueue);
 
 InitLeftClickState(&Editor->LeftClick, ArenaStore);
 InitVisualProfiler(&Editor->Profiler, Memory->Profiler);
 InitAnimatingCurvesState(&Editor->AnimatingCurves, Editor->EntityStore);
 InitMergingCurvesState(&Editor->MergingCurves, Editor->EntityStore);
 InitFrameStats(&Editor->FrameStats);
 InitProjectChangeRequestState(ArenaStore, &Editor->ProjectChange);
}

internal editor_last_sessions
LoadLastSessions(arena *Arena, b32 *SessionDirJustCreated)
{
 temp_arena Temp = TempArena(0);
 
 os_info Info = Platform.GetPlatformInfo();
 string EditorAppDir = PathConcat(Temp.Arena, Info.AppDir, EditorAppName);
 string LastSessionsFilePath = PathConcat(Arena, EditorAppDir, StrLit("session"));
 
 string_list ProjectFilePaths = {};
 
 if (OS_FileExists(EditorAppDir, true))
 {
  *SessionDirJustCreated = false;
  
  string LastSessionsData = OS_ReadEntireFile(Temp.Arena, LastSessionsFilePath);
  deserialize_stream Stream = MakeDeserializeStream(LastSessionsData);
  
  u32 ProjectFileCount = 0;
  DeserializeStruct(&Stream, &ProjectFileCount);
  ForEachIndex(ProjectIndex, ProjectFileCount)
  {
   string ProjectFilePath = DeserializeString(Arena, &Stream);
   StrListPush(Arena, &ProjectFilePaths, ProjectFilePath);
  }
 }
 else
 {
  OS_DirMake(EditorAppDir);
  *SessionDirJustCreated = true;
 }
 
 editor_last_sessions Result = {};
 Result.LastSessionsFilePath = LastSessionsFilePath;
 Result.ProjectFilePaths = ProjectFilePaths;
 
 EndTemp(Temp);
 
 return Result;
}

internal void
SaveLastSessions(editor_last_sessions LastSessions)
{
 temp_arena Temp = TempArena(0);
 string_list Paths = LastSessions.ProjectFilePaths;
 
 string_list List = {};
 u32 PathCount = SafeCastU32(Paths.NodeCount);
 SerializeStruct(Temp.Arena, &List, &PathCount);
 ListIter(Node, Paths.Head, string_list_node)
 {
  string Path = Node->Str;
  SerializeString(Temp.Arena, &List, Path);
 }
 
 string_list_join_options Opts = {};
 string Joined = StrListJoin(Temp.Arena, &List, Opts);
 OS_WriteDataToFile(LastSessions.LastSessionsFilePath, Joined);
 
 EndTemp(Temp);
}

internal void
AddSessionToLastSessions(arena *Arena, editor_last_sessions *LastSessions, string ProjectFilePath)
{
 string_list *Projects = &LastSessions->ProjectFilePaths;
 ListIter(Node, Projects->Head, string_list_node)
 {
  string Project = Node->Str;
  if (StrEqual(Project, ProjectFilePath))
  {
   StrListRemove(Projects, Node);
   break;
  }
 }
 
 string CopiedProjectFilePath = StrCopy(Arena, ProjectFilePath);
 StrListPush(Arena, Projects, CopiedProjectFilePath);
}

internal void
SetProjectFilePath(editor *Editor, b32 IsFileBacked, string FilePath)
{
 Editor->IsProjectFileBacked = IsFileBacked;
 ClearArena(Editor->ProjectFilePathArena);
 Editor->ProjectFilePath = StrCopy(Editor->ProjectFilePathArena, FilePath);
 
 UpdateWindowTitle(Editor);
 
 if (IsFileBacked)
 {
  editor_persistent_state *Persistent = &Editor->PersistentState;
  AddSessionToLastSessions(Persistent->Arena, &Persistent->LastSessions, FilePath);
  SaveLastSessions(Persistent->LastSessions);
 }
}

internal void
InitEditor(editor *Editor,
           editor_serializable_state SerializableState,
           b32 IsFileBacked,
           string FilePath)
{
 Editor->SerializableState = SerializableState;
 Editor->ProjectModified = false;
 SetProjectFilePath(Editor, IsFileBacked, FilePath);
}

internal void
LoadLastSessionOrEmptyProject(editor *Editor, editor_memory *Memory)
{
 arena *PermamentArena = Memory->PermamentArena;
 editor_persistent_state *Persistent = &Editor->PersistentState;
 
 b32 SessionDirJustCreated = false;
 editor_last_sessions LastSessions = LoadLastSessions(PermamentArena, &SessionDirJustCreated);
 
 Persistent->Memory = Memory;
 Persistent->Arena = PermamentArena;
 Persistent->LastSessions = LastSessions;
 
 if (SessionDirJustCreated)
 {
  // NOTE(hbr): This means editor has been opened on that machine for the first time. Show help.
  LoadEmptyProject(Editor);
  Editor->HelpWindow = true;
 }
 else
 {
  // NOTE(hbr): Try to load last session - otherwise, fallback to empty project
  b32 LastSessionLoadedSuccessfully = false;
  if (LastSessions.ProjectFilePaths.Tail)
  {
   string LastSession = LastSessions.ProjectFilePaths.Tail->Str;
   LastSessionLoadedSuccessfully = LoadProjectFromFile(Editor, LastSession);
  }
  if (!LastSessionLoadedSuccessfully)
  {
   LoadEmptyProject(Editor);
  }
 }
}

internal void
LoadEmptyProject(editor *Editor)
{
 AllocEditorResources(Editor);
 
 editor_serializable_state SerializableState = {};
 SerializableState.BackgroundColor = SerializableState.DefaultBackgroundColor = RGBA_U8(21, 21, 21);
 SerializableState.CollisionToleranceClip = 0.04f;
 SerializableState.RotationRadiusClip = 0.1f;
 SerializableState.CurveDefaultParams = DefaultCurveParams();
 SerializableState.EntityListWindow = true;
 SerializableState.SelectedEntityWindow = true;
#if BUILD_DEBUG
 SerializableState.DiagnosticsWindow = true;
 SerializableState.ProfilerWindow = true;
#endif
 InitCamera(&SerializableState.Camera);
 InitEditor(Editor, SerializableState, false, NilStr);
}

internal entity *
AllocEntityInternal(entity_store *Store, b32 DontTrack, entity *From)
{
 entity_list_node *Node = Store->Free;
 if (Node)
 {
  StackPop(Store->Free);
 }
 else
 {
  Node = PushStructNonZero(Store->Arena, entity_list_node);
 }
 
 entity *Entity = &Node->Entity;
 u32 Generation = Entity->Generation;
 StructZero(Entity);
 
 Entity->Id = Store->IdCounter++;
 Entity->Generation = Generation;
 
 curve *Curve = &Entity->Curve;
 image *Image = &Entity->Image;
 
 curve *FromCurve = &From->Curve;
 image *FromImage = &From->Image;
 
 if (From)
 {
  Entity->Name = From->Name;
  
  Curve->ParametricResources.MinT_Var.Equation = FromCurve->ParametricResources.MinT_Var.Equation;
  Curve->ParametricResources.MaxT_Var.Equation = FromCurve->ParametricResources.MaxT_Var.Equation;
  Curve->ParametricResources.X_Equation.Equation = FromCurve->ParametricResources.X_Equation.Equation;
  Curve->ParametricResources.Y_Equation.Equation = FromCurve->ParametricResources.Y_Equation.Equation;
  
  Image->FilePath = FromImage->FilePath;
 }
 else
 {
  Entity->Name = AllocStringOfSize(GetCtx()->StrStore, 128);
  
  Curve->ParametricResources.MinT_Var.Equation = AllocStringOfSize(GetCtx()->StrStore, 32);
  Curve->ParametricResources.MaxT_Var.Equation = AllocStringOfSize(GetCtx()->StrStore, 32);
  Curve->ParametricResources.X_Equation.Equation = AllocStringOfSize(GetCtx()->StrStore, 128);
  Curve->ParametricResources.Y_Equation.Equation = AllocStringOfSize(GetCtx()->StrStore, 128);
  
  Image->FilePath = AllocStringOfSize(GetCtx()->StrStore, 128);
 }
 
 // NOTE(hbr): It shouldn't really matter anyway that we allocate arenas even for
 // entities other than curves.
 Curve->Points = AllocCurvePointsFromStore(GetCtx()->CurvePointsStore);
 Curve->ComputeArena = AllocArenaFromStore(GetCtx()->ArenaStore, Megabytes(32));
 Curve->DegreeLowering.Arena = AllocArenaFromStore(GetCtx()->ArenaStore, Megabytes(32));
 
 if (!DontTrack)
 {
  ActivateEntity(Store, Entity);
 }
 
 return Entity;
}

internal entity *
AllocEntityFromEntity(entity_store *Store, entity *From)
{
 entity *Result = AllocEntityInternal(Store, false, From);
 return Result;
}

internal b32
LoadProjectFromFile(editor *Editor, string FilePath)
{
 b32 Success = false;
 temp_arena Temp = TempArena(0);
 
 string Data = OS_ReadEntireFile(Temp.Arena, FilePath);
 deserialize_stream Deserial = MakeDeserializeStream(Data);
 
 editor_save_header Header = {};
 editor_serializable_state SerializableState = {};
 
 DeserializeStruct(&Deserial, &Header);
 if (Header.MagicValue == EditorSaveFileMagicValue &&
     Header.Version == EditorVersion)
 {
  DeserializeStruct(&Deserial, &SerializableState);
  
  AllocEditorResources(Editor);
  InitEditor(Editor, SerializableState, true, FilePath);
  
  entity_store *EntityStore = GetCtx()->EntityStore;
  string_store *StrStore = GetCtx()->StrStore;
  curve_points_store *CurvePointsStore = GetCtx()->CurvePointsStore;
  
  for (u32 StrIndex = 0;
       StrIndex < Header.StringCount;
       ++StrIndex)
  {
   string String = DeserializeString(Temp.Arena, &Deserial);
   string_id Id = StringIdFromIndex(StrIndex);
   SetOrAllocStringOfId(StrStore, String, Id);
  }
  
  for (u32 CurvePointsIndex = 0;
       CurvePointsIndex < Header.CurvePointsCount;
       ++CurvePointsIndex)
  {
   u32 ControlPointCount = 0;
   v2 *ControlPoints = 0;
   f32 *ControlPointWeights;
   cubic_bezier_point *CubicBezierPoints = 0;
   u32 BSplineKnotCount = 0;
   f32 *BSplineKnots = 0;
   
   DeserializeStruct(&Deserial, &ControlPointCount);
   
   ControlPoints = PushArrayNonZero(Temp.Arena, ControlPointCount, v2);
   ControlPointWeights = PushArrayNonZero(Temp.Arena, ControlPointCount, f32);
   CubicBezierPoints = PushArrayNonZero(Temp.Arena, ControlPointCount, cubic_bezier_point);
   
   DeserializeArray(&Deserial, ControlPoints, ControlPointCount);
   DeserializeArray(&Deserial, ControlPointWeights, ControlPointCount);
   DeserializeArray(&Deserial, CubicBezierPoints, ControlPointCount);
   
   DeserializeStruct(&Deserial, &BSplineKnotCount);
   BSplineKnots = PushArrayNonZero(Temp.Arena, BSplineKnotCount, f32);
   DeserializeArray(&Deserial, BSplineKnots, BSplineKnotCount);
   
   curve_points_handle Points = MakeCurvePointsHandle(ControlPointCount,
                                                      ControlPoints,
                                                      ControlPointWeights,
                                                      CubicBezierPoints,
                                                      BSplineKnotCount,
                                                      BSplineKnots);
   curve_points_id Id = CurvePointsIdFromIndex(CurvePointsIndex);
   SetOrAllocCurvePointsOfId(CurvePointsStore, Id, Points);
  }
  
  ForEachIndex(EntityIndex, Header.EntityCount)
  {
   entity Serialized = {};
   DeserializeStruct(&Deserial, &Serialized);
   
   entity *Entity = AllocEntityFromEntity(EntityStore, &Serialized);
   switch (Serialized.Type)
   {
    case Entity_Curve: {
     entity_with_modify_witness Witness = BeginEntityModify(Entity);
     InitEntityFromEntity(&Witness, &Serialized, true);
     EndEntityModify(Witness);
    }break;
    
    case Entity_Image: {
     InitEntityPartFromEntity(Entity, &Serialized);
     string_id ImageFilePathId = Serialized.Image.FilePath;
     string ImageFilePath = StringFromStringId(StrStore, ImageFilePathId);
     image_instantiation_spec Spec = MakeImageInstantiationSpec_EntityProvided(Entity);
     TryLoadImage(Editor, ImageFilePath, Spec);
    }break;
    
    case Entity_Count: InvalidPath;
   }
  }
  
  Success = true;
 }
 
 EndTemp(Temp);
 
 return Success;
}

internal b32
SaveProjectIntoFile(editor *Editor, string FilePath)
{
 b32 Success = false;
 temp_arena Temp = TempArena(0);
 string_list Data = {};
 entity_store *EntityStore = GetCtx()->EntityStore;
 string_store *StrStore = GetCtx()->StrStore;
 curve_points_store *CurvePointsStore = GetCtx()->CurvePointsStore;
 entity_array Entities = AllEntityArrayFromStore(EntityStore);
 
 if (!StrEndsWith(FilePath, EditorSessionFileExtension))
 {
  FilePath = StrF(Temp.Arena, "%S.%S", FilePath, EditorSessionFileExtension);
 }
 
 editor_save_header Header = {};
 Header.MagicValue = EditorSaveFileMagicValue;
 Header.Version = EditorVersion;
 Header.StringCount = StrStore->StrCount;
 Header.EntityCount = Entities.Count;
 Header.CurvePointsCount = CurvePointsStore->Count;
 
 SerializeStruct(Temp.Arena, &Data, &Header);
 SerializeStruct(Temp.Arena, &Data, &Editor->SerializableState);
 
 for (u32 StrIndex = 0;
      StrIndex < Header.StringCount;
      ++StrIndex)
 {
  string_id Id = StringIdFromIndex(StrIndex);
  string String = StringFromStringId(StrStore, Id);
  SerializeString(Temp.Arena, &Data, String);
 }
 
 for (u32 CurvePointsIndex = 0;
      CurvePointsIndex < Header.CurvePointsCount;
      ++CurvePointsIndex)
 {
  curve_points_id Id = CurvePointsIdFromIndex(CurvePointsIndex);
  curve_points_static *Points = CurvePointsFromId(CurvePointsStore, Id);
  SerializeStruct(Temp.Arena, &Data, &Points->ControlPointCount);
  SerializeArray(Temp.Arena, &Data, Points->ControlPoints, Points->ControlPointCount);
  SerializeArray(Temp.Arena, &Data, Points->ControlPointWeights, Points->ControlPointCount);
  SerializeArray(Temp.Arena, &Data, Points->CubicBezierPoints, Points->ControlPointCount);
  SerializeStruct(Temp.Arena, &Data, &Points->BSplineKnotCount);
  SerializeArray(Temp.Arena, &Data, Points->BSplineKnots, Points->BSplineKnotCount);
 }
 
 ForEachIndex(EntityIndex, Header.EntityCount)
 {
  entity *Entity = Entities.Entities[EntityIndex];
  SerializeStruct(Temp.Arena, &Data, Entity);
 }
 
 // TODO(hbr): don't require to join this
 string_list_join_options Opts = {};
 string Joined = StrListJoin(Temp.Arena, &Data, Opts);
 Success = OS_WriteDataToFile(FilePath, Joined);
 
 if (Success)
 {
  MarkProjectAsUpToDate(Editor);
  SetProjectFilePath(Editor, true, FilePath);
 }
 
 EndTemp(Temp);
 
 return Success;
}

internal void
AddNotification(editor *Editor, notification_type Type, string Content)
{
 if (Editor->NotificationCount == MAX_NOTIFICATION_COUNT)
 {
  ArrayMove(Editor->Notifications, Editor->Notifications + 1, Editor->NotificationCount - 1);
  --Editor->NotificationCount;
 }
 Assert(Editor->NotificationCount < MAX_NOTIFICATION_COUNT);
 
 notification *Notification = Editor->Notifications + Editor->NotificationCount;
 ++Editor->NotificationCount;
 
 Notification->Type = Type;
 Notification->Content = StrCopy(Editor->NotificationsArena, Content);
 Notification->LifeTime = 0.0f;
 Notification->ScreenPosY = 0.0f;
}

internal void
AddNotificationF(editor *Editor, notification_type Type, char const *Format, ...)
{
 va_list Args;
 va_start(Args, Format);
 temp_arena Temp = TempArena(0);
 string Content = StrFV(Temp.Arena, Format, Args);
 AddNotification(Editor, Type, Content);
 EndTemp(Temp);
 va_end(Args);
}

internal void
DuplicateEntity(editor *Editor, entity *Entity)
{
 temp_arena Temp = TempArena(0);
 
 entity *Copy = AddEntity(Editor);
 entity_with_modify_witness CopyWitness = BeginEntityModify(Copy);
 string CopyName = StrF(Temp.Arena, "%S(copy)", GetEntityName(Entity));
 InitEntityFromEntity(&CopyWitness, Entity);
 SelectEntity(Editor, Copy);
 
 f32 SlightTranslationX = 5 * Copy->Curve.Params.DrawParams.Line.Width;
 v2 SlightTranslation = V2(SlightTranslationX, 0.0f);
 Copy->XForm.P += SlightTranslation;
 
 EndEntityModify(CopyWitness);
 EndTemp(Temp);
}

internal void
SplitCurveOnControlPoint(editor *Editor, entity *Entity)
{
 curve *Curve = SafeGetCurve(Entity);
 if (IsControlPointSelected(Curve))
 {
  temp_arena Temp = TempArena(0);
  
  curve_points_static *Points = CurvePointsFromId(GetCtx()->CurvePointsStore, Curve->Points);
  
  u32 SelectedIndex = IndexFromControlPointHandle(Curve->SelectedControlPoint);
  u32 HeadPointCount = SelectedIndex + 1;
  u32 TailPointCount = Points->ControlPointCount - SelectedIndex;
  
  entity *HeadEntity = AddEntity(Editor);
  entity *TailEntity = AddEntity(Editor);
  
  entity_with_modify_witness HeadWitness = BeginEntityModify(HeadEntity);
  entity_with_modify_witness TailWitness = BeginEntityModify(TailEntity);
  
  InitEntityFromEntity(&HeadWitness, Entity);
  InitEntityFromEntity(&TailWitness, Entity);
  
  curve *HeadCurve = SafeGetCurve(HeadEntity);
  curve *TailCurve = SafeGetCurve(TailEntity);
  
  string HeadName = StrF(Temp.Arena, "%S(head)", GetEntityName(Entity));
  string TailName = StrF(Temp.Arena, "%S(tail)", GetEntityName(Entity));
  SetEntityName(HeadEntity, HeadName);
  SetEntityName(TailEntity, TailName);
  
  curve_points_modify_handle HeadPoints = BeginModifyCurvePoints(&HeadWitness, HeadPointCount, ModifyCurvePointsWhichPoints_ControlPointsAndWeightsAndCubicBeziers);
  curve_points_modify_handle TailPoints = BeginModifyCurvePoints(&TailWitness, TailPointCount, ModifyCurvePointsWhichPoints_ControlPointsAndWeightsAndCubicBeziers);
  Assert(HeadPoints.PointCount == HeadPointCount);
  Assert(TailPoints.PointCount == TailPointCount);
  
  ArrayCopy(HeadPoints.ControlPoints, Points->ControlPoints, HeadPoints.PointCount);
  ArrayCopy(HeadPoints.Weights, Points->ControlPointWeights, HeadPoints.PointCount);
  ArrayCopy(HeadPoints.CubicBeziers, Points->CubicBezierPoints, HeadPoints.PointCount);
  
  u32 SplitAt = SelectedIndex;
  ArrayCopy(TailPoints.ControlPoints, Points->ControlPoints + SplitAt, TailPoints.PointCount);
  ArrayCopy(TailPoints.Weights, Points->ControlPointWeights + SplitAt, TailPoints.PointCount);
  ArrayCopy(TailPoints.CubicBeziers, Points->CubicBezierPoints + SplitAt, TailPoints.PointCount);
  
  EndModifyCurvePoints(HeadPoints);
  EndModifyCurvePoints(TailPoints);
  
  EndEntityModify(HeadWitness);
  EndEntityModify(TailWitness);
  
  RemoveEntity(Editor, Entity);
  
  EndTemp(Temp);
 }
}

internal void
ElevateBezierCurveDegree(editor *Editor, entity *Entity)
{
 curve *Curve = SafeGetCurve(Entity);
 Assert(IsRegularBezierCurve(Curve));
 entity_with_modify_witness Witness = BeginEntityModify(Entity);
 curve_points_store *CurvePointsStore = Editor->CurvePointsStore;
 curve_points_static *Points = CurvePointsFromId(CurvePointsStore, Curve->Points);
 u32 PointCount = Points->ControlPointCount;
 begin_modify_curve_points_static_tracked_result BeginModify = BeginModifyCurvePointsTracked(Editor, &Witness, PointCount + 1, ModifyCurvePointsWhichPoints_ControlPointsAndWeights);
 curve_points_modify_handle ModifyPoints = BeginModify.ModifyPoints;
 tracked_action *ModifyAction = BeginModify.ModifyAction;
 if (ModifyPoints.PointCount == PointCount + 1)
 {
  BezierCurveRational_EvelateDegree(ModifyPoints.ControlPoints,
                                    ModifyPoints.Weights,
                                    PointCount);
 }
 EndModifyCurvePointsTracked(Editor, ModifyAction, ModifyPoints);
 EndEntityModify(Witness);
}

internal vertex_array
CopyLineVertices(arena *Arena, vertex_array Vertices)
{
 vertex_array Result = Vertices;
 Result.Vertices = PushArrayNonZero(Arena, Vertices.VertexCount, v2);
 ArrayCopy(Result.Vertices, Vertices.Vertices, Vertices.VertexCount);
 
 return Result;
}

internal curve_points_static *
AllocCurvePoints(editor *Editor)
{
 curve_points_static_node *Node = Editor->FreeCurvePointsNode;
 if (Node)
 {
  StackPop(Editor->FreeCurvePointsNode);
 }
 else
 {
  Node = PushStructNonZero(Editor->Arena, curve_points_static_node);
 }
 curve_points_static *Points = &Node->Points;
 // NOTE(hbr): Don't [StructZero] because it is a big struct and it's not necessary
 Points->ControlPointCount = 0;
 return Points;
}

internal void
DeallocCurvePoints(editor *Editor, curve_points_static *Points)
{
 curve_points_static_node *Node = ContainerOf(Points, curve_points_static_node, Points);
 StackPush(Editor->FreeCurvePointsNode, Node);
}

internal entity *
GetSelectedEntity(editor *Editor)
{
 entity *Entity = EntityFromHandle(Editor->SelectedEntity);
 return Entity;
}

internal void
PerformBezierCurveSplit(editor *Editor, entity *Entity)
{
 temp_arena Temp = TempArena(0);
 
 curve *Curve = SafeGetCurve(Entity);
 point_tracking_along_curve_state *Tracking = &Curve->PointTracking;
 
 curve_points_static *Points = CurvePointsFromId(GetCtx()->CurvePointsStore, Curve->Points);
 u32 ControlPointCount = Points->ControlPointCount;
 
 Assert(Tracking->Type == PointTrackingAlongCurve_BezierCurveSplit);
 Tracking->Type = PointTrackingAlongCurve_None;
 
 entity *LeftEntity = AddEntity(Editor);
 entity *RightEntity = AddEntity(Editor);
 
 entity_with_modify_witness LeftWitness = BeginEntityModify(LeftEntity);
 entity_with_modify_witness RightWitness = BeginEntityModify(RightEntity);
 
 InitEntityFromEntity(&LeftWitness, Entity);
 InitEntityFromEntity(&RightWitness, Entity);
 
 string LeftName = StrF(Temp.Arena, "%S(left)", GetEntityName(Entity));
 string RightName = StrF(Temp.Arena, "%S(right)", GetEntityName(Entity));
 SetEntityName(LeftEntity, LeftName);
 SetEntityName(RightEntity, RightName);
 
 curve_points_modify_handle LeftPoints = BeginModifyCurvePoints(&LeftWitness, ControlPointCount, ModifyCurvePointsWhichPoints_ControlPointsAndWeights);
 curve_points_modify_handle RightPoints = BeginModifyCurvePoints(&RightWitness, ControlPointCount, ModifyCurvePointsWhichPoints_ControlPointsAndWeights);
 Assert(LeftPoints.PointCount == ControlPointCount);
 Assert(RightPoints.PointCount == ControlPointCount);
 
 BezierCurveSplit(Tracking->Fraction,
                  ControlPointCount, Points->ControlPoints, Points->ControlPointWeights,
                  LeftPoints.ControlPoints, LeftPoints.Weights,
                  RightPoints.ControlPoints, RightPoints.Weights);
 
 EndModifyCurvePoints(RightPoints);
 EndModifyCurvePoints(LeftPoints);
 
 EndEntityModify(LeftWitness);
 EndEntityModify(RightWitness);
 
 RemoveEntity(Editor, Entity);
 
 EndTemp(Temp);
}

internal void
BeginMergingCurves(merging_curves_state *Merging)
{
 Merging->Active = true;
 BeginChoosing2Curves(&Merging->Choose2Curves);
}

internal void
EndMergingCurves(editor *Editor, b32 Merged)
{
 merging_curves_state *Merging = &Editor->MergingCurves;
 if (Merged)
 {
  entity *Entity = AddEntity(Editor);
  entity_with_modify_witness EntityWitness = BeginEntityModify(Entity);
  InitEntityFromEntity(&EntityWitness, Merging->MergeEntity);
  EndEntityModify(EntityWitness);
  
  entity *Entity0 = EntityFromHandle(Merging->Choose2Curves.Curves[0]);
  if (Entity0)
  {
   RemoveEntity(Editor, Entity0);
  }
  entity *Entity1 = EntityFromHandle(Merging->Choose2Curves.Curves[1]);
  if (Entity1)
  {
   RemoveEntity(Editor, Entity1);
  }
 }
 Merging->Active = false;
}

internal b32
MergingWantsInput(merging_curves_state *Merging)
{
 b32 WantsInput = (Merging->Active && Merging->Choose2Curves.WaitingForChoice);
 return WantsInput;
}

internal void
MaybeReverseCurvePoints(entity *Entity)
{
 if (IsCurveReversed(Entity))
 {
  curve *Curve = SafeGetCurve(Entity);
  curve_points_static *Points = GetCurvePoints(Curve);
  ArrayReverse(Points->ControlPoints,       Points->ControlPointCount, v2);
  ArrayReverse(Points->ControlPointWeights, Points->ControlPointCount, f32);
  ArrayReverse(Points->CubicBezierPoints,   Points->ControlPointCount, cubic_bezier_point);
 }
}

internal bouncing_parameter
MakeBouncingParam(void)
{
 bouncing_parameter Result = {};
 Result.Speed = 0.5f;
 Result.Sign = 1.0f;
 
 return Result;
}

internal void
BeginAnimatingCurves(animating_curves_state *Animation)
{
 entity *ExtractEntity = Animation->ExtractEntity;
 StructZero(Animation);
 Animation->Flags = AnimatingCurves_Active;
 BeginChoosing2Curves(&Animation->Choose2Curves);
 Animation->Bouncing = MakeBouncingParam();
 Animation->ExtractEntity = ExtractEntity;
}

internal void
EndAnimatingCurves(animating_curves_state *Animation)
{
 Animation->Flags &= ~AnimatingCurves_Active;
}

internal b32
AnimationWantsInput(animating_curves_state *Animation)
{
 b32 WantsInput = ((Animation->Flags & AnimatingCurves_Active) && Animation->Choose2Curves.WaitingForChoice);
 return WantsInput;
}

internal void
BeginChoosing2Curves(choose_2_curves_state *Choosing)
{
 Choosing->WaitingForChoice = true;
 Choosing->ChoosingCurveIndex = 0;
 Choosing->Curves[0] = Choosing->Curves[1] = {};
}

internal b32
SupplyCurve(choose_2_curves_state *Choosing, entity *Curve)
{
 b32 AllSupplied = false;
 
 Choosing->Curves[Choosing->ChoosingCurveIndex] = MakeEntityHandle(Curve);
 if (EntityFromHandle(Choosing->Curves[0]) == 0)
 {
  Choosing->ChoosingCurveIndex = 0;
 }
 else if (EntityFromHandle(Choosing->Curves[1]) == 0)
 {
  Choosing->ChoosingCurveIndex = 1;
 }
 else
 {
  Choosing->WaitingForChoice = false;
  AllSupplied = true;
 }
 
 return AllSupplied;
}

internal void
BeginLoweringBezierCurveDegree(editor *Editor, curve_degree_lowering_state *Lowering)
{
 arena *Arena = Lowering->Arena;
 bezier_curve_degree_reduction_method Method = Lowering->Method;
 StructZero(Lowering);
 Lowering->Arena = Arena;
 Lowering->Stage = BezierCurveDegreeReductionStage_ChoosingMethod;
 Lowering->Method = Method;
}

internal void
EndLoweringBezierCurveDegree(editor *Editor,
                             curve_degree_lowering_state *Lowering,
                             b32 GoBack, string GoBackMsg)
{
 ClearArena(Lowering->Arena);
 if (Lowering->OriginalCurvePoints)
 {
  DeallocCurvePoints(Editor, Lowering->OriginalCurvePoints);
 }
 if (GoBack)
 {
  BeginLoweringBezierCurveDegree(Editor, Lowering);
  Lowering->DisplayMsg = GoBackMsg;
 }
 else
 {
  Lowering->Stage = BezierCurveDegreeReductionStage_NotActive;
 }
}

internal void
BeginEditorFrame(editor *Editor)
{
 if (!Editor->IsPendingActionTrackingGroup)
 {
  StructZero(&Editor->PendingActionTrackingGroup);
  Editor->IsPendingActionTrackingGroup = true;
 }
}

internal tracked_action *
AllocTrackedAction(editor *Editor)
{
 tracked_action *Action = Editor->FreeTrackedAction;
 if (Action)
 {
  StackPop(Editor->FreeTrackedAction);
 }
 else
 {
  Action = PushStructNonZero(Editor->Arena, tracked_action);
 }
 StructZero(Action);
 return Action;
}

internal void
DeallocTrackedAction(editor *Editor,
                     action_tracking_group *Group,
                     tracked_action *Action)
{
 DLLRemove(Group->ActionsHead, Group->ActionsTail, Action);
 StackPush(Editor->FreeTrackedAction, Action);
 if (Action->CurvePoints)
 {
  DeallocCurvePoints(Editor, Action->CurvePoints);
 }
 if (Action->FinalCurvePoints)
 {
  DeallocCurvePoints(Editor, Action->FinalCurvePoints);
 }
}

internal void
DeallocWholeActionTrackingGroup(editor *Editor, action_tracking_group *Group)
{
 ListIter(Action, Group->ActionsHead, tracked_action)
 {
  DeallocTrackedAction(Editor, Group, Action);
 }
}

internal b32
IsActionTrackingGroupEmpty(action_tracking_group *Group)
{
 b32 Empty = (Group->ActionsHead == 0);
 return Empty;
}

internal b32
IsActionTrackingGroupBatchFull(action_tracking_group_batch *Batch)
{
 b32 Result = (Batch->ActionTrackingGroupIndex == ArrayCount(Batch->ActionTrackingGroups));
 return Result;
}

internal void
EndActionTrackingGroup(editor *Editor, action_tracking_group *Group)
{
 if (!IsActionTrackingGroupEmpty(Group))
 {
  action_tracking_group_batch *CurrentBatch = Editor->CurrentActionTrackingGroupBatch;
  
  for (action_tracking_group_batch *Batch = CurrentBatch;
       (Batch && (Batch->ActionTrackingGroupCount != 0));
       Batch = Batch->Next)
  {
   // NOTE(hbr): Iterate through every action from the future (aka. actions that have been undo'ed)
   // and deallocate all AddEntity entities. Because they live in that future and that future only.
   // And we ditch that future and create a new path.
   for (u32 GroupIndex = Batch->ActionTrackingGroupIndex;
        GroupIndex < Batch->ActionTrackingGroupCount;
        ++GroupIndex)
   {
    action_tracking_group *Remove = Batch->ActionTrackingGroups + GroupIndex;
    ListIter(Action, Remove->ActionsHead, tracked_action)
    {
     if (Action->Type == TrackedAction_AddEntity)
     {
      entity *Entity = EntityFromHandle(Action->Entity, true);
      Assert(Entity);
      DeallocEntity(Editor->EntityStore, Entity);
     }
    }
    DeallocWholeActionTrackingGroup(Editor, Remove);
   }
   
   Batch->ActionTrackingGroupCount = Batch->ActionTrackingGroupIndex;
  }
  
  {  
   action_tracking_group_batch *Batch = CurrentBatch;
   
   if (Batch == 0 || IsActionTrackingGroupBatchFull(Batch))
   {
    if (Batch)
    {
     Batch = Batch->Next;
    }
   }
   if (Batch)
   {
    Assert(!IsActionTrackingGroupBatchFull(Batch));
   }
   else
   {
    Batch = PushStruct(Editor->Arena, action_tracking_group_batch);
    if (CurrentBatch)
    {
     CurrentBatch->Next = Batch;
    }
    Batch->Prev = CurrentBatch;
   }
   
   Batch->ActionTrackingGroups[Batch->ActionTrackingGroupIndex] = *Group;
   ++Batch->ActionTrackingGroupIndex;
   Batch->ActionTrackingGroupCount = Batch->ActionTrackingGroupIndex;
   
   Editor->CurrentActionTrackingGroupBatch = Batch;
  }
  
  // TODO(hbr): I'm not sure if this is the best place to do this
  MarkProjectAsModified(Editor);
 }
 else
 {
  DeallocWholeActionTrackingGroup(Editor, Group);
 }
}

internal project_change_request
MakeNoneProjectChangeRequest(void)
{
 project_change_request Request = {};
 Request.Type = ProjectChangeRequest_None;
 return Request;
}

internal project_change_request
MakeNewProjectChangeRequest(void)
{
 project_change_request Request = {};
 Request.Type = ProjectChangeRequest_NewProject;
 return Request;
}

internal project_change_request
MakeOpenFileProjectChangeRequest(void)
{
 project_change_request Request = {};
 Request.Type = ProjectChangeRequest_OpenFile;
 return Request;
}

internal project_change_request
MakeSaveProjectChangeRequest(void)
{
 project_change_request Request = {};
 Request.Type = ProjectChangeRequest_SaveProject;
 return Request;
}

internal project_change_request
MakeSaveAsProjectChangeRequest(void)
{
 project_change_request Request = {};
 Request.Type = ProjectChangeRequest_SaveProjectAs;
 return Request;
}

internal project_change_request
MakeQuitProjectChangeRequest(void)
{
 project_change_request Request = {};
 Request.Type = ProjectChangeRequest_Quit;
 return Request;
}

internal project_change_request
MakeLoadProjectProjectChangeRequest(string ProjectFilePath)
{
 project_change_request Request = {};
 Request.Type = ProjectChangeRequest_LoadProject;
 Request.ProjectFilePath = ProjectFilePath;
 return Request;
}

internal void
RequestProjectChange(editor *Editor, project_change_request Request)
{
 project_change_request_state *State = &Editor->ProjectChange;
 arena *Arena = State->Arena;
 StructZero(State);
 State->Arena = Arena;
 State->Request = Request;
}

internal void
ExecuteEditorCommand(editor *Editor, editor_command Cmd)
{
 switch (Cmd)
 {
  case EditorCommand_New: {RequestProjectChange(Editor, MakeNewProjectChangeRequest());}break;
  case EditorCommand_Open: {RequestProjectChange(Editor, MakeOpenFileProjectChangeRequest());}break;
  case EditorCommand_Save: {RequestProjectChange(Editor, MakeSaveProjectChangeRequest());}break;
  case EditorCommand_SaveAs: {RequestProjectChange(Editor, MakeSaveAsProjectChangeRequest());}break;
  case EditorCommand_Quit: {RequestProjectChange(Editor, MakeQuitProjectChangeRequest());}break;
  case EditorCommand_ToggleDevConsole: {Editor->DEBUG_Vars.DevConsole = !Editor->DEBUG_Vars.DevConsole;}break;
  case EditorCommand_ToggleDiagnostics: {Editor->DiagnosticsWindow = !Editor->DiagnosticsWindow;}break;
  case EditorCommand_ToggleProfiler: {Editor->Profiler.Stopped = !Editor->Profiler.Stopped;}break;
  case EditorCommand_Undo: {Undo(Editor);}break;
  case EditorCommand_Redo: {Redo(Editor);}break;
  case EditorCommand_ToggleUI: {Editor->HideUI = !Editor->HideUI;}break;
  
  case EditorCommand_Delete: {
   entity *Entity = GetSelectedEntity(Editor);
   entity_with_modify_witness EntityWitness = BeginEntityModify(Entity);
   if (Entity)
   {
    switch (Entity->Type)
    {
     case Entity_Curve: {
      curve *Curve = SafeGetCurve(Entity);
      if (IsControlPointSelected(Curve))
      {
       RemoveControlPoint(Editor, &EntityWitness, Curve->SelectedControlPoint);
      }
      else
      {
       RemoveEntity(Editor, Entity);
      }
     }break;
     
     case Entity_Image: {
      RemoveEntity(Editor, Entity);
     }break;
     
     case Entity_Count: InvalidPath;
    }
   }
   EndEntityModify(EntityWitness);
  }break;
  
  case EditorCommand_Duplicate: {
   entity *Entity = GetSelectedEntity(Editor);
   if (Entity)
   {
    DuplicateEntity(Editor, Entity);
   }
  }break;
  
  case EditorCommand_ToggleFullscreen: {
   Platform.ToggleFullscreen();
  }break;
  
  case EditorCommand_Count: InvalidPath;
 }
}

internal void
PushEditorCmd(editor *Editor, editor_command Command)
{
 editor_command_node *Node = Editor->FreeEditorCommandNode;
 if (Node)
 {
  StackPop(Editor->FreeEditorCommandNode);
 }
 else
 {
  Node = PushStructNonZero(Editor->Arena, editor_command_node);
 }
 StructZero(Node);
 Node->Command = Command;
 QueuePush(Editor->EditorCommandsHead, Editor->EditorCommandsTail, Node);
}

internal void
EndEditorFrame(editor *Editor)
{
 //- execute all editor commands gathered throughout the frame (mostly from input)
 ListIter(CmdNode, Editor->EditorCommandsHead, editor_command_node)
 {
  ExecuteEditorCommand(Editor, CmdNode->Command);
  StackPush(Editor->FreeEditorCommandNode, CmdNode);
 }
 Editor->EditorCommandsHead = 0;
 Editor->EditorCommandsTail = 0;
 //- end pending action tracking group is all tracked actions are completed
 if (Editor->IsPendingActionTrackingGroup)
 {
  action_tracking_group *Group = &Editor->PendingActionTrackingGroup;
  b32 HasPending = false;
  ListIter(Action, Group->ActionsHead, tracked_action)
  {
   if (Action->IsPending)
   {
    HasPending = true;
    break;
   }
  }
  if (!HasPending)
  {
   EndActionTrackingGroup(Editor, Group);
   Editor->IsPendingActionTrackingGroup = false;
  }
 }
}

internal action_tracking_group *
GetPendingActionTrackingGroup(editor *Editor)
{
 Assert(Editor->IsPendingActionTrackingGroup);
 return &Editor->PendingActionTrackingGroup;
}

internal void
BeginMovingEntity(editor_left_click_state *Left, editor *Editor, entity *Target)
{
 Left->Active = true;
 Left->Mode = EditorLeftClick_MovingEntity;
 Left->TargetEntity = MakeEntityHandle(Target);
 Left->PendingTrackedAction = BeginEntityTransform(Editor, Target);
}

internal void
BeginMovingCurvePoint(editor_left_click_state *Left,
                      editor *Editor,
                      entity *Target,
                      curve_point_handle CurvePoint)
{
 Left->Active = true;
 Left->Mode = EditorLeftClick_MovingCurvePoint;
 Left->TargetEntity = MakeEntityHandle(Target);
 Left->CurvePoint = CurvePoint;
 control_point_handle ControlPoint = ControlPointFromCurvePoint(CurvePoint);
 Left->PendingTrackedAction = BeginControlPointMove(Editor, Target, ControlPoint);
}

internal void
BeginMovingTrackingPoint(editor_left_click_state *Left, entity *Target)
{
 Left->Active = true;
 Left->Mode = EditorLeftClick_MovingTrackingPoint;
 Left->TargetEntity = MakeEntityHandle(Target);
}

internal void
BeginMovingBSplineKnot(editor_left_click_state *Left,
                       entity *Target,
                       b_spline_knot_handle BSplineKnot)
{
 Left->Active = true;
 Left->Mode = EditorLeftClick_MovingBSplineKnot;
 Left->TargetEntity = MakeEntityHandle(Target);
 Left->BSplineKnot = BSplineKnot;
}

internal void
EndLeftClick(editor *Editor, editor_left_click_state *Left)
{
 if (Left->Mode == EditorLeftClick_MovingCurvePoint)
 {
  EndControlPointMove(Editor, Left->PendingTrackedAction);
 }
 else if (Left->Mode == EditorLeftClick_MovingEntity)
 {
  EndEntityTransform(Editor, Left->PendingTrackedAction);
 }
 
 arena *OriginalVerticesArena = Left->OriginalVerticesArena;
 StructZero(Left);
 Left->OriginalVerticesArena = OriginalVerticesArena;
}

internal void
BeginMiddleClick(editor_middle_click_state *Middle, b32 Rotate, v2 ClipSpaceLastMouseP)
{
 Middle->Active = true;
 Middle->Rotate = Rotate;
 Middle->ClipSpaceLastMouseP = ClipSpaceLastMouseP;
}

internal void
EndMiddleClick(editor_middle_click_state *Middle)
{
 StructZero(Middle);
}

internal void
BeginRightClick(editor_right_click_state *Right, v2 ClickP, collision CollisionAtP)
{
 Right->Active = true;
 Right->ClickP = ClickP;
 Right->CollisionAtP = CollisionAtP;
}

internal void
EndRightClick(editor_right_click_state *Right)
{
 StructZero(Right);
}

internal void
ProfilerInspectAllFrames(visual_profiler_state *Visual)
{
 Visual->Mode = VisualProfilerMode_AllFrames;
}

internal void
ProfilerInspectSingleFrame(visual_profiler_state *Visual, u32 FrameIndex)
{
 profiler *Profiler = Visual->Profiler;
 Visual->Mode = VisualProfilerMode_SingleFrame;
 Assert(FrameIndex < MAX_PROFILER_FRAME_COUNT);
 Visual->FrameIndex = FrameIndex;
 Visual->FrameSnapshot = Profiler->Frames[FrameIndex];
}

internal tracked_action *
NextTrackedActionFromGroup(editor *Editor, action_tracking_group *Group, b32 Partial)
{
 tracked_action *Action = AllocTrackedAction(Editor);
 Action->IsPending = Partial;
 DLLPushBack(Group->ActionsHead, Group->ActionsTail, Action);
 return Action;
}

internal void
FinishPendingAction(editor *Editor, action_tracking_group *Group, tracked_action *Action, b32 Cancel)
{
 if (Action != &NilTrackedAction)
 {
  Assert(Action->IsPending);
  Action->IsPending = false;
  if (Cancel)
  {
   DeallocTrackedAction(Editor, Group, Action);
  }
 }
}

internal void
RemoveControlPoint(editor *Editor,
                   entity_with_modify_witness *Entity,
                   control_point_handle Point)
{
 action_tracking_group *Group = GetPendingActionTrackingGroup(Editor);
 tracked_action *Action = NextTrackedActionFromGroup(Editor, Group, false);
 Action->Type = TrackedAction_RemoveControlPoint;
 Action->Entity = MakeEntityHandle(Entity->Entity);
 Action->ControlPoint = GetCurveControlPointInWorldSpace(Entity->Entity, Point);
 Action->ControlPointHandle = Point;
 RemoveControlPoint(Entity, Point);
}

internal tracked_action *
BeginEntityTransform(editor *Editor, entity *Entity)
{
 action_tracking_group *Group = GetPendingActionTrackingGroup(Editor);
 tracked_action *Action = NextTrackedActionFromGroup(Editor, Group, true);
 Action->Type = TrackedAction_TransformEntity;
 Action->OriginalEntityXForm = Entity->XForm;
 Action->Entity = MakeEntityHandle(Entity);
 return Action;
}

internal void
EndEntityTransform(editor *Editor, tracked_action *MoveAction)
{
 action_tracking_group *Group = GetPendingActionTrackingGroup(Editor);
 entity *Entity = EntityFromHandle(MoveAction->Entity);
 b32 Cancel = false;
 if (Entity)
 {
  xform2d XForm = Entity->XForm;
  if (StructsEqual(&XForm, &MoveAction->OriginalEntityXForm))
  {
   Cancel = true;
  }
  else
  {
   MoveAction->XFormedToEntityXForm = XForm;
  }
 }
 FinishPendingAction(Editor, Group, MoveAction, Cancel);
}

internal tracked_action *
BeginControlPointMove(editor *Editor,
                      entity *Entity,
                      control_point_handle Point)
{
 action_tracking_group *Group = GetPendingActionTrackingGroup(Editor);
 tracked_action *Action = NextTrackedActionFromGroup(Editor, Group, true);
 Action->Type = TrackedAction_MoveControlPoint;
 Action->Entity = MakeEntityHandle(Entity);
 Action->ControlPointHandle = Point;
 Action->ControlPoint = GetCurveControlPointInWorldSpace(Entity, Point);
 return Action;
}

internal void
EndControlPointMove(editor *Editor, tracked_action *MoveAction)
{
 action_tracking_group *Group = GetPendingActionTrackingGroup(Editor);
 entity *Entity = EntityFromHandle(MoveAction->Entity);
 b32 Cancel = false;
 if (Entity)
 {
  control_point MovedToControlPoint = GetCurveControlPointInWorldSpace(Entity, MoveAction->ControlPointHandle);
  if (StructsEqual(&MoveAction->MovedToControlPoint, &MoveAction->ControlPoint))
  {
   Cancel = true;
  }
  else
  {
   MoveAction->MovedToControlPoint = MovedToControlPoint;
  }
 }
 FinishPendingAction(Editor, Group, MoveAction, Cancel);
}

internal begin_modify_curve_points_static_tracked_result
BeginModifyCurvePointsTracked(editor *Editor,
                              entity_with_modify_witness *Entity,
                              u32 RequestedPointCount,
                              modify_curve_points_static_which_points Which)
{
 action_tracking_group *Group = GetPendingActionTrackingGroup(Editor);
 tracked_action *Action = NextTrackedActionFromGroup(Editor, Group, true);
 Action->Type = TrackedAction_ModifyCurvePoints;
 Action->Entity = MakeEntityHandle(Entity->Entity);
 curve_points_static *CurvePoints = AllocCurvePoints(Editor);
 CopyCurvePointsFromCurve(SafeGetCurve(Entity->Entity), CurvePointsDynamicFromStatic(CurvePoints));
 Action->CurvePoints = CurvePoints;
 curve_points_modify_handle Handle = BeginModifyCurvePoints(Entity, RequestedPointCount, Which);
 begin_modify_curve_points_static_tracked_result Result = {};
 Result.ModifyPoints = Handle;
 Result.ModifyAction = Action;
 return Result;
}

internal void
EndModifyCurvePointsTracked(editor *Editor,
                            tracked_action *ModifyAction,
                            curve_points_modify_handle ModifyPoints)
{
 EndModifyCurvePoints(ModifyPoints);
 action_tracking_group *Group = GetPendingActionTrackingGroup(Editor);
 b32 Cancel = true;
 entity *Entity = EntityFromHandle(ModifyAction->Entity);
 if (Entity)
 {
  curve_points_static *CurvePoints = AllocCurvePoints(Editor);
  CopyCurvePointsFromCurve(SafeGetCurve(Entity), CurvePointsDynamicFromStatic(CurvePoints));
  ModifyAction->FinalCurvePoints = CurvePoints;
  Cancel = false;
 }
 FinishPendingAction(Editor, Group, ModifyAction, Cancel);
}

internal void
SetCurvePointsTracked(editor *Editor, entity_with_modify_witness *Entity, curve_points_handle Points)
{
 begin_modify_curve_points_static_tracked_result Modify = BeginModifyCurvePointsTracked(Editor, Entity, Points.ControlPointCount,
                                                                                        ModifyCurvePointsWhichPoints_ControlPointsAndWeightsAndCubicBeziers);
 curve_points_modify_handle ModifyPoints = Modify.ModifyPoints;
 tracked_action *ModifyAction = Modify.ModifyAction;
 SetCurvePoints(Entity, Points);
 EndModifyCurvePointsTracked(Editor, ModifyAction, ModifyPoints);
}

internal control_point_handle
AddControlPoint(editor *Editor,
                entity_with_modify_witness *Entity,
                v2 P, u32 At,
                b32 Append)
{
 action_tracking_group *Group = GetPendingActionTrackingGroup(Editor);
 tracked_action *Action = NextTrackedActionFromGroup(Editor, Group, false);
 Action->Type = TrackedAction_AddControlPoint;
 Action->Entity = MakeEntityHandle(Entity->Entity);
 control_point ControlPoint = MakeControlPoint(P);
 Action->ControlPoint = ControlPoint;
 control_point_handle Handle = (Append ? AppendControlPoint(Entity, P) : InsertControlPoint(Entity, ControlPoint, At));
 Action->ControlPointHandle = Handle;
 return Handle;
}

internal control_point_handle
InsertControlPoint(editor *Editor,
                   entity_with_modify_witness *Entity,
                   v2 P,
                   u32 At)
{
 control_point_handle Handle = AddControlPoint(Editor, Entity, P, At, false);
 return Handle;
}

internal control_point_handle
AppendControlPoint(editor *Editor,
                   entity_with_modify_witness *Entity,
                   v2 P)
{
 control_point_handle Result = AddControlPoint(Editor, Entity, P, 0, true);
 return Result;
}

internal void
RemoveEntity(editor *Editor, entity *Entity)
{
 action_tracking_group *Group = GetPendingActionTrackingGroup(Editor);
 tracked_action *Action = NextTrackedActionFromGroup(Editor, Group, false);
 Action->Type = TrackedAction_RemoveEntity;
 Action->Entity = MakeEntityHandle(Entity);
 DeactiveEntity(Editor->EntityStore, Entity);
}

internal entity *
AddEntity(editor *Editor)
{
 action_tracking_group *Group = GetPendingActionTrackingGroup(Editor);
 tracked_action *Action = NextTrackedActionFromGroup(Editor, Group, false);
 Action->Type = TrackedAction_AddEntity;
 entity *Entity = AllocEntity(Editor->EntityStore, false);
 Action->Entity = MakeEntityHandle(Entity);
 return Entity;
}

internal void
SelectEntity(editor *Editor, entity *Entity)
{
 //- mark currently selected entity as no longer selected
 entity *Current = EntityFromHandle(Editor->SelectedEntity);
 if (Current)
 {
  MarkEntityDeselected(Current);
 }
 //- select new entity
 if (Entity)
 {
  MarkEntitySelected(Entity);
 }
 Editor->SelectedEntity = MakeEntityHandle(Entity);
}

internal action_tracking_group_batch *
PrevActionTrackingGroupBatch(editor *Editor)
{
 action_tracking_group_batch *Batch = Editor->CurrentActionTrackingGroupBatch;
 if (Batch)
 {
  if (Batch->ActionTrackingGroupIndex == 0)
  {
   Batch = Batch->Prev;
  }
 }
 return Batch;
}

internal action_tracking_group_batch *
NextActionTrackingGroupBatch(editor *Editor)
{
 action_tracking_group_batch *Batch = Editor->CurrentActionTrackingGroupBatch;
 if (Batch)
 {
  if (Batch->ActionTrackingGroupIndex >= Batch->ActionTrackingGroupCount)
  {
   Batch = Batch->Next;
  }
 }
 return Batch;
}

internal void
Undo(editor *Editor)
{
 action_tracking_group_batch *Batch = PrevActionTrackingGroupBatch(Editor);
 if (Batch && Batch->ActionTrackingGroupIndex > 0)
 {
  Editor->CurrentActionTrackingGroupBatch = Batch;
  --Batch->ActionTrackingGroupIndex;
  action_tracking_group *Group = Batch->ActionTrackingGroups + Batch->ActionTrackingGroupIndex;
  
  ListIterRev(Action, Group->ActionsTail, tracked_action)
  {
   b32 AllowDeactived = (Action->Type == TrackedAction_RemoveEntity);
   entity *Entity = EntityFromHandle(Action->Entity, AllowDeactived);
   // NOTE(hbr): Deselecting entity if zero
   b32 AllowZero = (Action->Type == TrackedAction_SelectEntity);
   if (Entity || AllowZero)
   {
    entity_with_modify_witness Witness = BeginEntityModify(Entity);
    switch (Action->Type)
    {
     case TrackedAction_RemoveControlPoint: {
      u32 InsertAt = IndexFromControlPointHandle(Action->ControlPointHandle);
      InsertControlPoint(&Witness, Action->ControlPoint, InsertAt);
     }break;
     
     case TrackedAction_AddControlPoint: {
      RemoveControlPoint(&Witness, Action->ControlPointHandle);
     }break;
     
     case TrackedAction_MoveControlPoint: {
      SetControlPoint(&Witness, Action->ControlPointHandle, Action->ControlPoint);
     }break;
     
     case TrackedAction_TransformEntity: {
      Entity->XForm = Action->OriginalEntityXForm;
     }break;
     
     case TrackedAction_RemoveEntity: {
      ActivateEntity(Editor->EntityStore, Entity);
     }break;
     
     case TrackedAction_AddEntity: {
      DeactiveEntity(Editor->EntityStore, Entity);
     }break;
     
     case TrackedAction_SelectEntity: {
      entity *Previous = EntityFromHandle(Action->PreviouslySelectedEntity);
      SelectEntity(Editor, Previous);
     }break;
     
     case TrackedAction_ModifyCurvePoints: {
      curve_points_static *Points = Action->CurvePoints;
      SetCurvePoints(&Witness, CurvePointsHandleFromCurvePointsStatic(Points));
     }break;
    }
    EndEntityModify(Witness);
   }
  }
 }
}

internal void
Redo(editor *Editor)
{
 action_tracking_group_batch *Batch = NextActionTrackingGroupBatch(Editor);
 if (Batch && Batch->ActionTrackingGroupIndex < Batch->ActionTrackingGroupCount)
 {
  Editor->CurrentActionTrackingGroupBatch = Batch;
  action_tracking_group *Group = Batch->ActionTrackingGroups + Batch->ActionTrackingGroupIndex;
  ++Batch->ActionTrackingGroupIndex;
  
  ListIter(Action, Group->ActionsHead, tracked_action)
  {
   b32 AllowDeactived = (Action->Type == TrackedAction_AddEntity);
   entity *Entity = EntityFromHandle(Action->Entity, AllowDeactived);
   // NOTE(hbr): Deselecting entity if zero
   b32 AllowZero = (Action->Type == TrackedAction_SelectEntity);
   if (Entity || AllowZero)
   {
    entity_with_modify_witness Witness = BeginEntityModify(Entity);
    switch (Action->Type)
    {
     case TrackedAction_RemoveControlPoint: {
      RemoveControlPoint(&Witness, Action->ControlPointHandle);
     }break;
     
     case TrackedAction_AddControlPoint: {
      u32 InsertAt = IndexFromControlPointHandle(Action->ControlPointHandle);
      InsertControlPoint(&Witness, Action->ControlPoint, InsertAt);
     }break;
     
     case TrackedAction_MoveControlPoint: {
      SetControlPoint(&Witness, Action->ControlPointHandle, Action->MovedToControlPoint);
     }break;
     
     case TrackedAction_TransformEntity: {
      Entity->XForm = Action->XFormedToEntityXForm;
     }break;
     
     case TrackedAction_RemoveEntity: {
      DeactiveEntity(Editor->EntityStore, Entity);
     }break;
     
     case TrackedAction_AddEntity: {
      ActivateEntity(Editor->EntityStore, Entity);
     }break;
     
     case TrackedAction_SelectEntity: {
      SelectEntity(Editor, Entity);
     }break;
     
     case TrackedAction_ModifyCurvePoints: {
      curve_points_static *Points = Action->FinalCurvePoints;
      SetCurvePoints(&Witness, CurvePointsHandleFromCurvePointsStatic(Points));
     }break;
    }
    EndEntityModify(Witness);
   }
  }
 }
}

internal u32
InternalIndexFromTextureHandle(entity_store *Store, render_texture_handle Handle)
{
 u32 Index = TextureIndexFromHandle(Handle) - 1;
 Assert(Index < Store->TextureCount);
 return Index;
}

internal render_texture_handle
TextureHandleFromInternalIndex(u32 Index)
{
 render_texture_handle Handle = TextureHandleFromIndex(Index + 1);
 return Handle;
}

internal render_texture_handle
AllocTextureHandle(entity_store *Store)
{
 render_texture_handle Result = TextureHandleZero();
 for (u32 Index = 0;
      Index < Store->TextureCount;
      ++Index)
 {
  if (Store->TextureHandleRefCount[Index] == 0)
  {
   Result = TextureHandleFromInternalIndex(Index);
   ++Store->TextureHandleRefCount[Index];
   break;
  }
 }
 return Result;
}

internal void
DeallocTextureHandle(entity_store *Store, render_texture_handle Handle)
{
 if (!TextureHandleMatch(Handle, TextureHandleZero()))
 {
  u32 Index = InternalIndexFromTextureHandle(Store, Handle);
  Assert(Store->TextureHandleRefCount[Index] != 0);
  --Store->TextureHandleRefCount[Index];
 }
}

internal render_texture_handle
CopyTextureHandle(entity_store *Store, render_texture_handle Handle)
{
 u32 Index = InternalIndexFromTextureHandle(Store, Handle);
 ++Store->TextureHandleRefCount[Index];
 return Handle;
}

internal entity_store *
AllocEntityStore(arena_store *ArenaStore,
                 u32 MaxTextureCount,
                 u32 MaxBufferCount)
{
 arena *Arena = AllocArenaFromStore(ArenaStore, Gigabytes(1));
 entity_store *Store = PushStruct(Arena, entity_store);
 Store->Arena = Arena;
 ForEachElement(Index, Store->ByTypeArenas)
 {
  Store->ByTypeArenas[Index] = AllocArenaFromStore(ArenaStore, Megabytes(1));
 }
 Store->TextureCount = MaxTextureCount;
 Store->TextureHandleRefCount = PushArray(Arena, MaxTextureCount, b32);
 Store->BufferCount = MaxBufferCount;
 Store->IsBufferHandleAllocated = PushArray(Arena, MaxBufferCount, b32);
 return Store;
}

internal curve_points_store *
AllocCurvePointsStore(arena_store *ArenaStore)
{
 arena *Arena = AllocArenaFromStore(ArenaStore, Gigabytes(1));
 curve_points_store *Store = PushStruct(Arena, curve_points_store);
 Store->Arena = Arena;
 Store->ArenaStore = ArenaStore;
 return Store;
}

internal curve_points_id
CurvePointsIdFromIndex(u32 Index)
{
 curve_points_id Id = {};
 Id.Index = Index;
 return Id;
}

internal u32
IndexFromCurvePointsId(curve_points_id Id)
{
 u32 Index = Id.Index;
 return Index;
}

internal b32
CurvePointsIdMatch(curve_points_id A, curve_points_id B)
{
 b32 Result = (A.Index == B.Index);
 return Result;
}

internal curve_points_id
AllocCurvePointsFromStore(curve_points_store *Store)
{
 if (Store->Count >= Store->Capacity)
 {
  u32 NewCapacity = Max(Store->Count + 1, 2 * Store->Capacity);
  curve_points_static *NewCurvePoints = PushArrayNonZero(Store->Arena, NewCapacity, curve_points_static);
  ArrayCopy(NewCurvePoints, Store->CurvePoints, Store->Count);
  Store->CurvePoints = NewCurvePoints;
  Store->Capacity = NewCapacity;
 }
 Assert(Store->Count < Store->Capacity);
 u32 Index = Store->Count++;
 curve_points_id Id = CurvePointsIdFromIndex(Index);
 return Id;
}

internal void
SetOrAllocCurvePointsOfId(curve_points_store *Store,
                          curve_points_id Id,
                          curve_points_handle Points)
{
 u32 Index = IndexFromCurvePointsId(Id);
 if (Index >= Store->Count)
 {
  curve_points_id AllocatedId = AllocCurvePointsFromStore(Store);
  Assert(CurvePointsIdMatch(Id, AllocatedId));
 }
 curve_points_static *Dst = CurvePointsFromId(Store, Id);
 CopyCurvePoints(CurvePointsDynamicFromStatic(Dst), Points);
}

internal curve_points_static *
CurvePointsFromId(curve_points_store *Store, curve_points_id Id)
{
 u32 Index = IndexFromCurvePointsId(Id);
 Assert(Index < Store->Count);
 curve_points_static *Points = Store->CurvePoints + Index;
 return Points;
}

internal entity *
AllocEntity(entity_store *Store, b32 DontTrack)
{
 entity *Result = AllocEntityInternal(Store, DontTrack, 0);
 return Result;
}

volatile u32 GlobalCounter = 0;

internal void
DeallocEntity(entity_store *Store, entity *Entity)
{
 curve *Curve = &Entity->Curve;
 image *Image = &Entity->Image;
 
 DeallocArenaFromStore(GetCtx()->ArenaStore, Curve->ComputeArena);
 DeallocArenaFromStore(GetCtx()->ArenaStore, Curve->DegreeLowering.Arena);
 DeallocStringFromStore(GetCtx()->StrStore, Curve->ParametricResources.X_Equation.Equation);
 DeallocStringFromStore(GetCtx()->StrStore, Curve->ParametricResources.Y_Equation.Equation);
 
 GlobalCounter += Image->TextureHandle.U32[0];
 
 DeallocTextureHandle(Store, Image->TextureHandle);
 
 DeactiveEntity(Store, Entity);
 
 entity_list_node *Node = ContainerOf(Entity, entity_list_node, Entity);
 StackPush(Store->Free, Node);
}

internal void
ActivateEntity(entity_store *Store, entity *Entity)
{
 entity_list_node *Node = ContainerOf(Entity, entity_list_node, Entity);
 DLLPushBack(Store->Head, Store->Tail, Node);
 ++Store->Count;
 ++Store->AllocGeneration;
 Entity->InternalFlags |= EntityInternalFlag_Tracked;
 // NOTE(hbr): If I'm activating entity that was previously deactivated,
 // decremented its previously incremented generation.
 if (Entity->InternalFlags & EntityInternalFlag_Deactivated)
 {
  Assert(Entity->Generation > 0);
  --Entity->Generation;
 }
 Entity->InternalFlags &= ~EntityInternalFlag_Deactivated;
}

internal void
DeactiveEntity(entity_store *Store, entity *Entity)
{
 entity_list_node *Node = ContainerOf(Entity, entity_list_node, Entity);
 if (Entity->InternalFlags & EntityInternalFlag_Tracked)
 {
  DLLRemove(Store->Head, Store->Tail, Node);
  --Store->Count;
  ++Store->AllocGeneration;
  Entity->InternalFlags &= ~EntityInternalFlag_Tracked;
 }
 Entity->InternalFlags |= EntityInternalFlag_Deactivated;
 ++Entity->Generation;
}

internal entity_array
AllEntityArrayFromStore(entity_store *Store)
{
 entity_array Array = EntityArrayFromType(Store, Entity_Count);
 return Array;
}

internal entity_array
EntityArrayFromType(entity_store *Store, entity_type Type)
{
 if (Store->AllocGeneration != Store->ByTypeGenerations[Type])
 {
  arena *Arena = Store->ByTypeArenas[Type];
  ClearArena(Arena);
  
  entity_array Array = {};
  u32 MaxCount = Store->Count;
  Array.Entities = PushArrayNonZero(Arena, MaxCount, entity *);
  entity **At = Array.Entities;
  ListIter(Node, Store->Head, entity_list_node)
  {
   entity *Entity = &Node->Entity;
   if (Type == Entity_Count || (Type == Entity->Type))
   {
    *At++ = Entity;
   }
  }
  u32 ActualCount = Cast(u32)(At - Array.Entities);
  Assert(ActualCount <= MaxCount);
  Array.Count = ActualCount;
  
  Store->ByTypeArrays[Type] = Array;
  Store->ByTypeGenerations[Type] = Store->AllocGeneration;
 }
 return Store->ByTypeArrays[Type];
}

internal thread_task_memory_store *
AllocThreadTaskMemoryStore(arena_store *ArenaStore)
{
 arena *Arena = AllocArenaFromStore(ArenaStore, Gigabytes(1));
 thread_task_memory_store *Store = PushStruct(Arena, thread_task_memory_store);
 Store->Arena = Arena;
 Store->ArenaStore = ArenaStore;
 return Store;
}

internal thread_task_memory *
BeginThreadTaskMemory(thread_task_memory_store *Store)
{
 thread_task_memory *Task = Store->Free;
 if (Task)
 {
  StackPop(Store->Free);
 }
 else
 {
  Task = PushStructNonZero(Store->Arena, thread_task_memory);
 }
 StructZero(Task);
 Task->Arena = AllocArenaFromStore(Store->ArenaStore, Gigabytes(1));
 return Task;
}

internal void
EndThreadTaskMemory(thread_task_memory_store *Store, thread_task_memory *Task)
{
 DeallocArenaFromStore(Store->ArenaStore, Task->Arena);
 StackPush(Store->Free, Task);
}

internal image_loading_store *
AllocImageLoadingStore(arena_store *ArenaStore)
{
 arena *Arena = AllocArenaFromStore(ArenaStore, Gigabytes(1));
 image_loading_store *Store = PushStruct(Arena, image_loading_store);
 Store->Arena = Arena;
 Store->ArenaStore = ArenaStore;
 return Store;
}

internal void
DeallocImageLoadingStore(image_loading_store *Store)
{
 DeallocArenaFromStore(Store->ArenaStore, Store->Arena);
 StructZero(Store);
}

internal image_loading_task *
BeginAsyncImageLoadingTask(image_loading_store *Store)
{
 image_loading_task *Task = Store->Free;
 if (Task)
 {
  StackPop(Store->Free);
 }
 else
 {
  Task = PushStructNonZero(Store->Arena, image_loading_task);
 }
 StructZero(Task);
 
 Task->Arena = AllocArenaFromStore(Store->ArenaStore, Gigabytes(1));
 DLLPushBack(Store->Head, Store->Tail, Task);
 
 return Task;
}

internal void
FinishAsyncImageLoadingTask(image_loading_store *Store, image_loading_task *Task)
{
 DeallocArenaFromStore(Store->ArenaStore, Task->Arena);
 DLLRemove(Store->Head, Store->Tail, Task);
 StackPush(Store->Free, Task);
}

internal image_instantiation_spec
MakeImageInstantiationSpec_EntityProvided(entity *Entity)
{
 image_instantiation_spec Spec = {};
 Spec.Type = ImageInstantiationSpec_EntityProvided;
 Spec.ImageEntity = MakeEntityHandle(Entity);
 return Spec;
}

internal image_instantiation_spec
MakeImageInstantiationSpec_AtP(v2 AtP)
{
 image_instantiation_spec Spec = {};
 Spec.Type = ImageInstantiationSpec_AtP;
 Spec.InstantiateImageAtP = AtP;
 return Spec;
}

internal multiline_collision
CheckCollisionWithMultiLine(v2 LocalAtP, v2 *CurveSamples, u32 PointCount, f32 Width, f32 Tolerance)
{
 multiline_collision Result = {};
 
 f32 CheckWidth = Width + Tolerance;
 f32 MinDistance = 0.0f;
 u32 MinDistancePointIndex = 0;
 for (u32 PointIndex = 0;
      PointIndex + 1 < PointCount;
      ++PointIndex)
 {
  v2 P1 = CurveSamples[PointIndex + 0];
  v2 P2 = CurveSamples[PointIndex + 1];
  f32 Distance = SegmentSignedDistance(LocalAtP, P1, P2, CheckWidth);
  if (Distance < MinDistance)
  {
   MinDistance = Distance;
   MinDistancePointIndex = PointIndex;
  }
 }
 
 if (MinDistance < 0)
 {
  Result.Collided = true;
  Result.PointIndex = MinDistancePointIndex;
 }
 
 return Result;
}

internal sort_entry_array
SortEntities(arena *Arena, entity_array Entities)
{
 sort_entry_array SortArray = AllocSortEntryArray(Arena, Entities.Count, SortOrder_Descending);
 for (u32 EntityIndex = 0;
      EntityIndex < Entities.Count;
      ++EntityIndex)
 {
  entity *Entity = Entities.Entities[EntityIndex];
  // NOTE(hbr): Equal sorting layer images should be below curves
  f32 Offset = 0.0f;
  switch (Entity->Type)
  {
   case Entity_Image: {Offset = 0.5f;} break;
   case Entity_Curve: {Offset = 0.0f;} break;
   case Entity_Count: InvalidPath; break;
  }
  
  AddSortEntry(&SortArray, Entity->SortingLayer + Offset, EntityIndex);
 }
 
 Sort(SortArray.Entries, SortArray.Count, SortFlag_Stable);
 
 return SortArray;
}

internal collision
CheckCollisionWithEntities(editor *Editor, v2 AtP, f32 Tolerance)
{
 ProfileFunctionBegin();
 
 entity_array Entities = AllEntityArrayFromStore(Editor->EntityStore);
 
 collision Result = {};
 temp_arena Temp = TempArena(0);
 sort_entry_array Sorted = SortEntities(Temp.Arena, Entities);
 
 for (u64 SortedIndex = 0;
      SortedIndex < Sorted.Count;
      ++SortedIndex)
 {
  u64 InverseIndex = Sorted.Count-1 - SortedIndex;
  u32 EntityIndex = Sorted.Entries[InverseIndex].Index;
  entity *Entity = Entities.Entities[EntityIndex];
  
  if (IsEntityVisible(Entity))
  {
   v2 LocalAtP = WorldToLocalEntityPosition(Entity, AtP);
   // TODO(hbr): Note that Tolerance becomes wrong when we want to compute everything in
   // local space, fix that
   switch (Entity->Type)
   {
    case Entity_Curve: {
     curve *Curve = &Entity->Curve;
     curve_params *Params = &Curve->Params;
     curve_points_store *CurvePointsStore = Editor->CurvePointsStore;
     curve_points_static *Points = CurvePointsFromId(CurvePointsStore, Curve->Points);
     v2 *ControlPoints = Points->ControlPoints;
     u32 ControlPointCount = Points->ControlPointCount;
     v2 *BSplinePartitionKnots = Curve->BSplinePartitionKnots;
     point_tracking_along_curve_state *Tracking = &Curve->PointTracking;
     
     if (AreCurvePointsVisible(Curve))
     {
      //- control points
      {
       f32 MinSignedDistance = F32_INF;
       u32 MinPointIndex = 0;
       for (u32 PointIndex = 0;
            PointIndex < ControlPointCount;
            ++PointIndex)
       {
        // TODO(hbr): Maybe move this function call outside of this loop
        // due to performance reasons.
        point_draw_info PointInfo = GetCurveControlPointDrawInfo(Entity, ControlPointHandleFromIndex(PointIndex));
        f32 CollisionRadius = PointInfo.Radius + PointInfo.OutlineThickness + Tolerance;
        f32 SignedDistance = PointDistanceSquaredSigned(LocalAtP, ControlPoints[PointIndex], CollisionRadius);
        if (SignedDistance < MinSignedDistance)
        {
         MinSignedDistance = SignedDistance;
         MinPointIndex = PointIndex;
        }
       }
       
       if (MinSignedDistance < 0)
       {
        Result.Entity = Entity;
        Result.Flags |= Collision_CurvePoint;
        Result.CurvePoint = CurvePointFromControlPoint(ControlPointHandleFromIndex(MinPointIndex));
       }
      }
      
      //- bezier "control" points
      {
       visible_cubic_bezier_points VisibleBeziers = GetVisibleCubicBezierPoints(Entity);
       f32 MinSignedDistance = F32_INF;
       cubic_bezier_point_handle MinPoint = {};
       
       for (u32 Index = 0;
            Index < VisibleBeziers.Count;
            ++Index)
       {
        f32 PointRadius = GetCurveCubicBezierPointRadius(Curve);
        cubic_bezier_point_handle Bezier = VisibleBeziers.Indices[Index];
        v2 BezierPoint = GetCubicBezierPoint(Curve, Bezier);
        f32 SignedDistance = PointDistanceSquaredSigned(LocalAtP, BezierPoint, PointRadius + Tolerance);
        if (SignedDistance < MinSignedDistance)
        {
         MinSignedDistance = SignedDistance;
         MinPoint = Bezier;
        }
       }
       
       if (MinSignedDistance < 0)
       {
        Result.Entity = Entity;
        Result.Flags |= Collision_CurvePoint;
        Result.CurvePoint = CurvePointFromCubicBezierPoint(MinPoint);
       }
      }
     }
     
     if (Tracking->Type != PointTrackingAlongCurve_None)
     {
      f32 PointRadius = GetCurveTrackedPointRadius(Curve);
      if (PointCollision(LocalAtP, Tracking->LocalSpaceTrackedPoint, PointRadius + Tolerance))
      {
       Result.Entity = Entity;
       Result.Flags |= Collision_TrackedPoint;
      }
      
      // NOTE(hbr): !Collision.Entity - small optimization because this collision doesn't add
      // anything to Flags.
      if ((Tracking->Type == PointTrackingAlongCurve_DeCasteljauVisualization) && !Result.Entity)
      {
       u32 IterationCount = Tracking->Intermediate.IterationCount;
       v2 *Points = Tracking->Intermediate.P;
       
       u32 PointIndex = 0;
       for (u32 Iteration = 0;
            Iteration < IterationCount;
            ++Iteration)
       {
        v2 *CurveSamples = Points + PointIndex;
        u32 PointCount = IterationCount - Iteration;
        multiline_collision Line = CheckCollisionWithMultiLine(LocalAtP,
                                                               CurveSamples,
                                                               PointCount,
                                                               Params->DrawParams.Line.Width,
                                                               Tolerance);
        if (Line.Collided)
        {
         Result.Entity = Entity;
         goto collided_label;
        }
        
        for (u32 I = 0; I < PointCount; ++I)
        {
         if (PointCollision(LocalAtP, Points[PointIndex], Params->DrawParams.Points.Radius + Tolerance))
         {
          Result.Entity = Entity;
          goto collided_label;
         }
         ++PointIndex;
        }
       }
       collided_label:
       NoOp;
      }
     }
     
     if (Params->DrawParams.Line.Enabled)
     {
      multiline_collision Line = CheckCollisionWithMultiLine(LocalAtP,
                                                             Curve->CurveSamples,
                                                             Curve->CurveSampleCount,
                                                             Params->DrawParams.Line.Width,
                                                             Tolerance);
      if (Line.Collided)
      {
       Result.Entity = Entity;
       Result.Flags |= Collision_CurveLine;
       Result.CurveSampleIndex = Line.PointIndex;
      }
     }
     
     // NOTE(hbr): !Collision.Entity - small optimization because this collision doesn't add
     // anything to Flags.
     if (IsConvexHullVisible(Curve) && !Result.Entity)
     {
      multiline_collision Line = CheckCollisionWithMultiLine(LocalAtP,
                                                             Curve->ConvexHullPoints,
                                                             Curve->ConvexHullCount,
                                                             Params->DrawParams.ConvexHull.Width,
                                                             Tolerance);
      if (Line.Collided)
      {
       Result.Entity = Entity;
      }
     }
     
     // NOTE(hbr): !Collision.Entity - small optimization because this collision doesn't add
     // anything to Flags.
     if (IsPolylineVisible(Curve) && !Result.Entity)
     {
      multiline_collision Line = CheckCollisionWithMultiLine(LocalAtP,
                                                             ControlPoints,
                                                             ControlPointCount,
                                                             Params->DrawParams.Polyline.Width,
                                                             Tolerance);
      if (Line.Collided)
      {
       Result.Entity = Entity;
      }
     }
     
     if (AreBSplineKnotsVisible(Curve))
     {
      u32 PartitionSize = GetBSplineParams(Curve).KnotParams.PartitionSize;
      v2 *PartitionKnotPoints = BSplinePartitionKnots;
      point_draw_info KnotPointInfo = GetBSplinePartitionKnotPointDrawInfo(Entity);
      
      // NOTE(hbr): Skip first and last points because they are not moveable
      for (u32 PartitionKnotIndex = 1;
           PartitionKnotIndex + 1 < PartitionSize;
           ++PartitionKnotIndex)
      {
       v2 Knot = PartitionKnotPoints[PartitionKnotIndex];
       if (PointCollision(LocalAtP, Knot, KnotPointInfo.Radius))
       {
        Result.Entity = Entity;
        Result.Flags |= Collision_BSplineKnot;
        Result.BSplineKnot = BSplineKnotHandleFromPartitionKnotIndex(Curve, PartitionKnotIndex);
        break;
       }
      }
     }
    } break;
    
    case Entity_Image: {
     image *Image = &Entity->Image;
     scale2d Dim = Image->Dim;
     if (-Dim.V.X <= LocalAtP.X && LocalAtP.X <= Dim.V.X &&
         -Dim.V.Y <= LocalAtP.Y && LocalAtP.Y <= Dim.V.Y)
     {
      Result.Entity = Entity;
     }
    } break;
    
    case Entity_Count: InvalidPath; break;
   }
  }
  
  if (Result.Entity)
  {
   break;
  }
 }
 
 EndTemp(Temp);
 
 ProfileEnd();
 
 return Result;
}

inline internal entity_snapshot_for_merging
MakeEntitySnapshotForMerging(entity *Entity)
{
 entity_snapshot_for_merging Result = {};
 Result.Entity = Entity;
 if (Entity)
 {
  Result.XForm = Entity->XForm;
  Result.Flags = Entity->Flags;
  Result.Version = Entity->Version;
 }
 return Result;
}

inline internal b32
EntityModified(entity_snapshot_for_merging Versioned, entity *Entity)
{
 b32 Result = false;
 if (Versioned.Entity != Entity ||
     (Entity && (!StructsEqual(&Entity->XForm, &Versioned.XForm) ||
                 !StructsEqual(&Entity->Flags, &Versioned.Flags) ||
                 !StructsEqual(&Entity->Version, &Versioned.Version))))
 {
  Result = true;
 }
 return Result;
}

inline internal entity_handle
MakeEntityHandle(entity *Entity)
{
 entity_handle Handle = {};
 if (Entity)
 {
  Handle.Entity = Entity;
  Handle.Generation = Entity->Generation;
 }
 return Handle;
}

internal entity_handle
EntityHandleZero(void)
{
 entity_handle Handle = {};
 return Handle;
}

inline internal entity *
EntityFromHandle(entity_handle Handle, b32 AllowDeactived)
{
 entity *Entity = 0;
 if (Handle.Entity &&
     (Handle.Generation == Handle.Entity->Generation))
 {
  Entity = Handle.Entity;
 }
 if (AllowDeactived &&
     (Entity == 0) &&
     (Handle.Entity->InternalFlags & EntityInternalFlag_Deactivated) &&
     (Handle.Generation + 1 == Handle.Entity->Generation))
 {
  Entity = Handle.Entity;
 }
 return Entity;
}

internal inline f32
GetCurvePartVisibilityZOffset(curve_part_visibility Part)
{
 Assert(Part < CurvePartVisibility_Count);
 f32 Result = Cast(f32)(CurvePartVisibility_Count-1 - Part) / CurvePartVisibility_Count;
 return Result;
}

inline internal curve *
SafeGetCurve(entity *Entity)
{
 Assert(Entity->Type == Entity_Curve);
 return &Entity->Curve;
}

inline internal image *
SafeGetImage(entity *Entity)
{
 Assert(Entity->Type == Entity_Image);
 return &Entity->Image;
}

inline internal control_point_handle
ControlPointHandleFromIndex(u32 Index)
{
 control_point_handle Handle = {};
 Handle.Index = Index + 1;
 return Handle;
}

inline internal u32
IndexFromControlPointHandle(control_point_handle Handle)
{
 u32 Result = 0;
 if (!ControlPointHandleMatch(Handle, ControlPointHandleZero()))
 {
  Result = Handle.Index - 1;
 }
 return Result;
}

inline internal b32
IndexFromControlPointHandleSafe(control_point_handle Handle, u32 *Out)
{
 b32 Result = false;
 if (!ControlPointHandleMatch(Handle, ControlPointHandleZero()))
 {
  *Out = Handle.Index - 1;
  Result = true;
 }
 return Result;
}

inline internal b32
ControlPointHandleMatch(control_point_handle A, control_point_handle B)
{
 b32 Result = (A.Index == B.Index);
 return Result;
}

inline internal control_point_handle
ControlPointHandleZero(void)
{
 control_point_handle Index = {};
 return Index;
}

inline internal control_point_handle
ControlPointFromCubicBezierPoint(cubic_bezier_point_handle Bezier)
{
 control_point_handle Result = {};
 if (!CubicBezierPointHandleMatch(Bezier, CubicBezierPointHandleZero()))
 {
  u32 BezierIndex = IndexFromCubicBezierPointHandle(Bezier);
  Result = ControlPointHandleFromIndex(BezierIndex / 3);
 }
 return Result;
}

inline internal cubic_bezier_point_handle
CubicBezierPointHandleFromIndex(u32 Index)
{
 cubic_bezier_point_handle Handle = {};
 Handle.Index = Index + 1;
 return Handle;
}

inline internal cubic_bezier_point_handle
CubicBezierPointFromControlPoint(control_point_handle Handle)
{
 cubic_bezier_point_handle Bezier = {};
 if (!ControlPointHandleMatch(Handle, ControlPointHandleZero()))
 {
  u32 I = IndexFromControlPointHandle(Handle);
  Bezier = CubicBezierPointHandleFromIndex(3 * I + 1);
 }
 return Bezier;
}

inline internal u32
IndexFromCubicBezierPointHandle(cubic_bezier_point_handle Handle)
{
 Assert(!CubicBezierPointHandleMatch(Handle, CubicBezierPointHandleZero()));
 u32 Index = Handle.Index - 1;
 return Index;
}

inline internal b32
CubicBezierPointHandleMatch(cubic_bezier_point_handle A, cubic_bezier_point_handle B)
{
 b32 Equal = (A.Index == B.Index);
 return Equal;
}

inline internal cubic_bezier_point_handle
CubicBezierPointHandleZero(void)
{
 cubic_bezier_point_handle Handle = {};
 return Handle;
}

inline internal curve_point_handle
CurvePointFromControlPoint(control_point_handle Handle)
{
 curve_point_handle Result = {};
 Result.Type = CurvePoint_ControlPoint;
 Result.Control = Handle;
 return Result;
}

inline internal curve_point_handle
CurvePointFromCubicBezierPoint(cubic_bezier_point_handle Handle)
{
 curve_point_handle Result = {};
 Result.Type = CurvePoint_CubicBezierPoint;
 Result.Bezier = Handle;
 return Result;
}

inline internal control_point_handle
ControlPointFromCurvePoint(curve_point_handle Handle)
{
 control_point_handle Result = {};
 switch (Handle.Type)
 {
  case CurvePoint_ControlPoint: {Result = Handle.Control;}break;
  case CurvePoint_CubicBezierPoint: {Result = ControlPointFromCubicBezierPoint(Handle.Bezier);}break;
 }
 return Result;
}

inline internal v2
GetCubicBezierPoint(curve *Curve, cubic_bezier_point_handle Point)
{
 v2 Result = {};
 u32 Index = IndexFromCubicBezierPointHandle(Point);
 curve_points_static *Points = GetCurvePoints(Curve);
 if (Index < 3 * Points->ControlPointCount)
 {
  v2 *Beziers = Cast(v2 *)Points->CubicBezierPoints;
  Result = Beziers[Index];
 }
 return Result;
}

inline internal v2
GetControlPointP(curve *Curve, control_point_handle Point)
{
 v2 Result = {};
 u32 Index = IndexFromControlPointHandle(Point);
 curve_points_static *Points = GetCurvePoints(Curve);
 if (Index < Points->ControlPointCount)
 {
  Result = Points->ControlPoints[Index];
 }
 return Result;
}

inline internal v2
GetCenterPointFromCubicBezierPoint(curve *Curve, cubic_bezier_point_handle Bezier)
{
 control_point_handle Control = ControlPointFromCubicBezierPoint(Bezier);
 v2 Point = GetControlPointP(Curve, Control);
 return Point;
}

inline internal control_point_handle
ControlPointIndexFromCurveSampleIndex(curve *Curve, u32 CurveSampleIndex)
{
 Assert(!IsCurveTotalSamplesMode(Curve));
 u32 Index = SafeDiv0(CurveSampleIndex, Curve->Params.SamplesPerControlPoint);
 curve_points_static *Points = GetCurvePoints(Curve);
 Assert(Index < Points->ControlPointCount);
 Index = ClampTop(Index, Points->ControlPointCount - 1);
 control_point_handle Control = ControlPointHandleFromIndex(Index);
 return Control;
}

internal b32
AreCurvePointsVisible(curve *Curve)
{
 b32 Result = (Curve->Params.DrawParams.Points.Enabled && UsesControlPoints(Curve));
 return Result;
}

internal b32
IsPolylineVisible(curve *Curve)
{
 curve_params *CurveParams = &Curve->Params;
 b32 Result = (CurveParams->DrawParams.Polyline.Enabled && UsesControlPoints(Curve));
 return Result;
}

internal b32
IsConvexHullVisible(curve *Curve)
{
 curve_params *CurveParams = &Curve->Params;
 b32 Result = (CurveParams->DrawParams.ConvexHull.Enabled && UsesControlPoints(Curve));
 return Result;
}

internal b32
IsBSplineCurve(curve *Curve)
{
 curve_params *CurveParams = &Curve->Params;
 b32 Result = (CurveParams->Type == Curve_NURBS);
 return Result;
}

internal b32
AreBSplineConvexHullsVisible(curve *Curve)
{
 curve_params *CurveParams = &Curve->Params;
 b32 Result = (IsBSplineCurve(Curve) && CurveParams->DrawParams.BSplinePartialConvexHull.Enabled);
 return Result;
}

internal b32
UsesControlPoints(curve *Curve)
{
 b32 Result = false;
 switch (Curve->Params.Type)
 {
  case Curve_CubicSpline:
  case Curve_Bezier:
  case Curve_Polynomial:
  case Curve_NURBS: {Result = true;}break;
  
  case Curve_Parametric: {}break;
  
  case Curve_Count: InvalidPath;
 }
 
 return Result;
}

internal b32
IsEntityVisible(entity *Entity)
{
 b32 Result = !(Entity->Flags & EntityFlag_Hidden);
 return Result;
}

internal void
SetEntityVisibility(entity *Entity, b32 Visible)
{
 if (Visible)
 {
  Entity->Flags &= ~EntityFlag_Hidden;
 }
 else
 {
  Entity->Flags |= EntityFlag_Hidden;
 }
}

internal b32
IsEntitySelected(entity *Entity)
{
 b32 Result = (Entity->Flags & EntityFlag_Selected);
 return Result;
}

internal f32
GetCurveTrackedPointRadius(curve *Curve)
{
 f32 Result = 1.5f * Curve->Params.DrawParams.Line.Width;
 return Result;
}

internal void
AddVisibleCubicBezierPoint(visible_cubic_bezier_points *Visible, cubic_bezier_point_handle Point)
{
 Assert(Visible->Count < ArrayCount(Visible->Indices));
 Visible->Indices[Visible->Count] = Point;
 ++Visible->Count;
}

inline internal b32
PrevCubicBezierPoint(cubic_bezier_point_handle *Point)
{
 b32 Result = false;
 u32 Index = IndexFromCubicBezierPointHandle(*Point);
 if (Index > 0)
 {
  *Point = CubicBezierPointHandleFromIndex(Index - 1);
  Result = true;
 }
 return Result;
}

inline internal b32
NextCubicBezierPoint(curve *Curve, cubic_bezier_point_handle *Point)
{
 b32 Result = false;
 u32 Index = IndexFromCubicBezierPointHandle(*Point);
 curve_points_static *Points = GetCurvePoints(Curve);
 if (Index + 1 < 3 * Points->ControlPointCount)
 {
  *Point = CubicBezierPointHandleFromIndex(Index + 1);
  Result = true;
 }
 return Result;
}

internal visible_cubic_bezier_points
GetVisibleCubicBezierPoints(entity *Entity)
{
 visible_cubic_bezier_points Result = {};
 
 curve *Curve = SafeGetCurve(Entity);
 if (IsEntitySelected(Entity) &&
     IsControlPointSelected(Curve) &&
     Curve->Params.Type == Curve_Bezier &&
     Curve->Params.Bezier == Bezier_CubicSpline)
 {
  cubic_bezier_point_handle StartPoint = CubicBezierPointFromControlPoint(Curve->SelectedControlPoint);
  
  cubic_bezier_point_handle PrevPoint = StartPoint;
  if (PrevCubicBezierPoint(&PrevPoint))
  {
   AddVisibleCubicBezierPoint(&Result, PrevPoint);
   if (PrevCubicBezierPoint(&PrevPoint))
   {
    AddVisibleCubicBezierPoint(&Result, PrevPoint);
   }
  }
  
  cubic_bezier_point_handle NextPoint = StartPoint;
  if (NextCubicBezierPoint(Curve, &NextPoint))
  {
   AddVisibleCubicBezierPoint(&Result, NextPoint);
   if (NextCubicBezierPoint(Curve, &NextPoint))
   {
    AddVisibleCubicBezierPoint(&Result, NextPoint);
   }
  }
 }
 
 return Result;
}

internal b32
IsCurveEligibleForPointTracking(curve *Curve)
{
 curve_params *Params = &Curve->Params;
 curve_type Interpolation = Params->Type;
 bezier_type BezierVariant = Params->Bezier;
 b32 Result = (Interpolation == Curve_Bezier && BezierVariant == Bezier_Rational);
 
 return Result;
}

internal b32
CurveHasWeights(curve *Curve)
{
 curve_params *P = &Curve->Params;
 b32 Result = ((P->Type == Curve_Bezier && P->Bezier == Bezier_Rational) ||
               (P->Type == Curve_NURBS));
 return Result;
}

internal b32
IsCurveTotalSamplesMode(curve *Curve)
{
 b32 Result = false;
 switch (Curve->Params.Type)
 {
  case Curve_CubicSpline:
  case Curve_Bezier:
  case Curve_Polynomial:
  case Curve_NURBS: {}break;
  
  case Curve_Parametric: {
   Result = true;
  }break;
  
  case Curve_Count: InvalidPath;
 }
 
 return Result;
}

inline internal b32
IsCurve(entity *Entity)
{
 b32 Result = (Entity->Type == Entity_Curve);
 return Result;
}

internal b32
IsCurveReversed(entity *Entity)
{
 curve *Curve = SafeGetCurve(Entity);
 b32 Result = (UsesControlPoints(Curve) && (Entity->Flags & EntityFlag_CurveAppendFront));
 return Result;
}

internal b32
IsRegularBezierCurve(curve *Curve)
{
 b32 Result = (Curve->Params.Type == Curve_Bezier && Curve->Params.Bezier == Bezier_Rational);
 return Result;
}

internal b32
AreBSplineKnotsVisible(curve *Curve)
{
 b32 Result = (Curve->Params.Type == Curve_NURBS && Curve->Params.DrawParams.BSplineKnots.Enabled);
 return Result;
}

internal curve_merge_compatibility
AreCurvesCompatibleForMerging(curve *Curve0, curve *Curve1, curve_merge_method Method)
{
 b32 Compatible = false;
 string WhyIncompatible = NilStr;
 
 curve_params *Params0 = &Curve0->Params;
 curve_params *Params1 = &Curve1->Params;
 
 string IncompatibleTypes = StrLit("cannot merge curves of different types");
 
#define Concat_C0_G1_Str " can only be merged using [Concat], [C0] or [G1] methods"
#define Concat_Str       " can only be merged using [Concat] method"
 
 b32 Concat_C0_G1 = (Method == CurveMerge_Concat ||
                     Method == CurveMerge_C0 ||
                     Method == CurveMerge_G1);
 b32 Concat = (Method == CurveMerge_Concat);
 
 if (Params0->Type == Params1->Type)
 {
  switch (Params0->Type)
  {
   case Curve_Polynomial: {
    if (Concat_C0_G1) Compatible = true;
    else WhyIncompatible = StrLit("polynomial curves" Concat_C0_G1_Str);
   }break;
   
   case Curve_CubicSpline: {
    if (Params0->CubicSpline == Params1->CubicSpline)
    {
     if (Concat_C0_G1) Compatible = true;
     else WhyIncompatible = StrLit("cubic spline curves" Concat_C0_G1_Str);
    }
    else
    {
     WhyIncompatible = IncompatibleTypes;
    }
   }break;
   
   case Curve_Bezier: {
    if (Params0->Bezier == Params1->Bezier)
    {
     switch (Params0->Bezier)
     {
      case Bezier_Rational: {Compatible = true;}break;
      case Bezier_CubicSpline: {
       if (Concat_C0_G1) Compatible = true;
       else WhyIncompatible = StrLit("cubic bezier curves" Concat_C0_G1_Str);
      }break;
      case Bezier_Count: InvalidPath;
     }
    }
    else
    {
     WhyIncompatible = IncompatibleTypes;
    }
   }break;
   
   case Curve_NURBS: {
    if (Concat) Compatible = true;
    else WhyIncompatible = StrLit("B-spline curves" Concat_Str);
   }break;
   
   case Curve_Parametric: {WhyIncompatible = StrLit("parametric curves cannot be merged");}break;
   
   case Curve_Count: InvalidPath;
  }
 }
 else
 {
  WhyIncompatible = IncompatibleTypes;
 }
 
 curve_merge_compatibility Result = {};
 Result.Compatible = Compatible;
 Result.WhyIncompatible = WhyIncompatible;
 
 return Result;
}

internal void
MarkEntitySelected(entity *Entity)
{
 Entity->Flags |= EntityFlag_Selected;
}

internal void
MarkEntityDeselected(entity *Entity)
{
 Entity->Flags &= ~EntityFlag_Selected;
}

internal point_draw_info
GetEntityPointDrawInfo(entity *Entity, f32 BaseRadius, rgba BaseColor)
{
 point_draw_info Result = {};
 
 Result.Radius = BaseRadius;
 
 f32 OutlineScale = 0.0f;
 if (IsEntitySelected(Entity))
 {
  OutlineScale = 0.55f;
 }
 Result.OutlineThickness = OutlineScale * Result.Radius;
 
 Result.Color = BaseColor;
 Result.OutlineColor = RGBA_Brighten(Result.Color, 0.3f);
 
 return Result;
}

internal point_draw_info
GetCurveControlPointDrawInfo(entity *Entity, control_point_handle Point)
{
 curve *Curve = SafeGetCurve(Entity);
 curve_params *Params = &Curve->Params;
 u32 Index = IndexFromControlPointHandle(Point);
 
 point_draw_info Result = GetEntityPointDrawInfo(Entity, Params->DrawParams.Points.Radius, Params->DrawParams.Points.Color);
 curve_points_static *Points = GetCurvePoints(Curve);
 
 if (( (Entity->Flags & EntityFlag_CurveAppendFront) && Index == 0) ||
     (!(Entity->Flags & EntityFlag_CurveAppendFront) && Index == Points->ControlPointCount-1))
 {
  Result.Radius *= 2.0f;
 }
 
 if (IsEntitySelected(Entity) && ControlPointHandleMatch(Point, Curve->SelectedControlPoint))
 {
  //Result.OutlineColor = RGBA_Brighten(Result.OutlineColor, 0.5f);
  
  Result.Color = RGBA_Opposite(Result.Color);
  Result.OutlineColor = RGBA_Brighten(Result.Color, 0.5f);
 }
 
 return Result;
}

internal point_draw_info
GetBSplinePartitionKnotPointDrawInfo(entity *Entity)
{
 curve *Curve = SafeGetCurve(Entity);
 curve_params *Params = &Curve->Params;
 draw_params *BSplineKnots = &Params->DrawParams.BSplineKnots;
 point_draw_info Result = GetEntityPointDrawInfo(Entity, BSplineKnots->Radius, BSplineKnots->Color);
 return Result;
}

internal f32
GetCurveCubicBezierPointRadius(curve *Curve)
{
 // TODO(hbr): Make bezier points smaller
 f32 Result = Curve->Params.DrawParams.Points.Radius;
 return Result;
}

internal string
GetEntityName(entity *Entity)
{
 string Name = StringFromStringId(GetCtx()->StrStore, Entity->Name);
 return Name;
}

internal char_buffer *
GetEntityNameBuffer(entity *Entity, string_store *StrStore)
{
 char_buffer *Buffer = CharBufferFromStringId(StrStore, Entity->Name);
 return Buffer;
}

internal control_point
MakeControlPoint(v2 Point)
{
 control_point Result = {};
 Result.P = Point;
 return Result;
}

internal control_point
MakeControlPoint(v2 Point, f32 Weight, cubic_bezier_point Bezier)
{
 control_point Result = {};
 Result.P = Point;
 Result.Weight = Weight;
 Result.Bezier = Bezier;
 Result.IsWeight = Result.IsBezier = true;
 return Result;
}

internal curve_points_handle
MakeCurvePointsHandle(u32 ControlPointCount,
                      v2 *ControlPoints,
                      f32 *ControlPointWeights,
                      cubic_bezier_point *CubicBezierPoints,
                      u32 BSplineKnotCount,
                      f32 *BSplineKnots)
{
 curve_points_handle Points = {};
 Points.ControlPointCount = ControlPointCount;
 Points.ControlPoints = ControlPoints;
 Points.ControlPointWeights = ControlPointWeights;
 Points.CubicBezierPoints = CubicBezierPoints;
 Points.BSplineKnotCount = BSplineKnotCount;
 Points.BSplineKnots = BSplineKnots;
 return Points;
}

internal curve_points_handle
CurvePointsHandleZero(void)
{
 curve_points_handle Handle = {};
 return Handle;
}

internal curve_points_handle
CurvePointsHandleFromCurvePointsStatic(curve_points_static *Static)
{
 curve_points_handle Handle = MakeCurvePointsHandle(Static->ControlPointCount,
                                                    Static->ControlPoints,
                                                    Static->ControlPointWeights,
                                                    Static->CubicBezierPoints,
                                                    Static->BSplineKnotCount,
                                                    Static->BSplineKnots);
 return Handle;
}

internal curve_points_dynamic
MakeCurvePointsDynamic(u32 *ControlPointCount,
                       v2 *ControlPoints,
                       f32 *ControlPointWeights,
                       cubic_bezier_point *CubicBezierPoints,
                       u32 CapacityPoints,
                       u32 *BSplineKnotCount,
                       f32 *BSplineKnots,
                       u32 CapacityKnots)
{
 curve_points_dynamic Result = {};
 Result.ControlPointCount = ControlPointCount;
 Result.ControlPoints = ControlPoints;
 Result.ControlPointWeights = ControlPointWeights;
 Result.CubicBezierPoints = CubicBezierPoints;
 Result.CapacityPoints = CapacityPoints;
 Result.BSplineKnotCount = BSplineKnotCount;
 Result.BSplineKnots = BSplineKnots;
 Result.CapacityKnots = CapacityKnots;
 return Result;
}

internal curve_points_dynamic
CurvePointsDynamicFromStatic(curve_points_static *Static)
{
 curve_points_dynamic Dynamic = MakeCurvePointsDynamic(&Static->ControlPointCount,
                                                       Static->ControlPoints,
                                                       Static->ControlPointWeights,
                                                       Static->CubicBezierPoints,
                                                       CURVE_POINTS_STATIC_MAX_CONTROL_POINT_COUNT,
                                                       &Static->BSplineKnotCount,
                                                       Static->BSplineKnots,
                                                       MAX_B_SPLINE_KNOT_COUNT);
 return Dynamic;
}

internal control_point
GetCurveControlPointInWorldSpace(entity *Entity, control_point_handle Point)
{
 curve *Curve = SafeGetCurve(Entity);
 u32 Index = IndexFromControlPointHandle(Point);
 curve_points_static *Points = GetCurvePoints(Curve);
 
 v2 P = LocalToWorldEntityPosition(Entity, Points->ControlPoints[Index]);
 f32 Weight = Points->ControlPointWeights[Index];
 cubic_bezier_point B = Points->CubicBezierPoints[Index];
 cubic_bezier_point Bezier = {
  LocalToWorldEntityPosition(Entity, B.Ps[0]),
  LocalToWorldEntityPosition(Entity, B.Ps[1]),
  LocalToWorldEntityPosition(Entity, B.Ps[2]),
 };
 
 control_point Result = MakeControlPoint(P, Weight, Bezier);
 return Result;
}

internal void
CopyCurvePointsFromCurve(curve *Curve, curve_points_dynamic Dst)
{
 curve_points_static *Points = GetCurvePoints(Curve);
 CopyCurvePoints(Dst, CurvePointsHandleFromCurvePointsStatic(Points));
}

internal void
CopyCurvePoints(curve_points_dynamic Dst, curve_points_handle Src)
{
 u32 CopyPoints = Min(Src.ControlPointCount, Dst.CapacityPoints);
 *Dst.ControlPointCount = CopyPoints;
 ArrayCopy(Dst.ControlPoints, Src.ControlPoints, CopyPoints);
 ArrayCopy(Dst.ControlPointWeights, Src.ControlPointWeights, CopyPoints);
 ArrayCopy(Dst.CubicBezierPoints, Src.CubicBezierPoints, CopyPoints);
 
 u32 CopyKnots = Min(Src.BSplineKnotCount, Dst.CapacityKnots);
 *Dst.BSplineKnotCount = CopyKnots;
 ArrayCopy(Dst.BSplineKnots, Src.BSplineKnots, CopyKnots);
}

internal rect2
EntityAABB(entity *Entity)
{
 rect2 AABB = EmptyAABB();
 switch (Entity->Type)
 {
  case Entity_Curve: {
   curve *Curve = &Entity->Curve;
   u32 SampleCount = Curve->CurveSampleCount;
   v2 *Samples = Curve->CurveSamples;
   u32 SampleIndex = 0;
   while (SampleIndex + 8 < SampleCount)
   {
    AddPointAABB(&AABB, Samples[SampleIndex + 0]);
    AddPointAABB(&AABB, Samples[SampleIndex + 1]);
    AddPointAABB(&AABB, Samples[SampleIndex + 2]);
    AddPointAABB(&AABB, Samples[SampleIndex + 3]);
    AddPointAABB(&AABB, Samples[SampleIndex + 4]);
    AddPointAABB(&AABB, Samples[SampleIndex + 5]);
    AddPointAABB(&AABB, Samples[SampleIndex + 6]);
    AddPointAABB(&AABB, Samples[SampleIndex + 7]);
    SampleIndex += 8;
   }
   while (SampleIndex < SampleCount)
   {
    AddPointAABB(&AABB, Samples[SampleIndex]);
    ++SampleIndex;
   }
  } break;
  
  case Entity_Image: {
   image *Image = SafeGetImage(Entity);
   scale2d Dim = Image->Dim;
   AddPointAABB(&AABB, V2( Dim.V.X,  Dim.V.Y));
   AddPointAABB(&AABB, V2( Dim.V.X, -Dim.V.Y));
   AddPointAABB(&AABB, V2(-Dim.V.X,  Dim.V.Y));
   AddPointAABB(&AABB, V2(-Dim.V.X, -Dim.V.Y));
  } break;
  
  case Entity_Count: InvalidPath; break;
 }
 
 rect2_corners AABB_Corners = AABBCorners(AABB);
 rect2 AABB_Transformed = EmptyAABB();
 ForEachEnumVal(Corner, Corner_Count, corner)
 {
  v2 P = LocalToWorldEntityPosition(Entity, AABB_Corners.Corners[Corner]);
  AddPointAABB(&AABB_Transformed, P);
 }
 
 return AABB_Transformed;
}

internal b_spline_params
GetBSplineParams(curve *Curve)
{
 return Curve->ComputedBSplineParams;
}

internal b32
BSplineKnotHandleMatch(b_spline_knot_handle A, b_spline_knot_handle B)
{
 b32 Result = (A.__Index == B.__Index);
 return Result;
}

internal b_spline_knot_handle
BSplineKnotHandleZero(void)
{
 b_spline_knot_handle Handle = {};
 return Handle;
}

internal u32
KnotIndexFromBSplineKnotHandle(b_spline_knot_handle Handle)
{
 Assert(!BSplineKnotHandleMatch(Handle, BSplineKnotHandleZero()));
 u32 Index = Handle.__Index - 1;
 return Index;
}

internal u32
PartitionKnotIndexFromBSplineKnotHandle(curve *Curve, b_spline_knot_handle Handle)
{
 u32 KnotIndex = KnotIndexFromBSplineKnotHandle(Handle);
 b_spline_params BSplineParams = GetBSplineParams(Curve);
 b_spline_knot_params KnotParams = BSplineParams.KnotParams;
 Assert(KnotIndex >= KnotParams.Degree);
 u32 PartitionKnotIndex = KnotIndex - KnotParams.Degree;
 return PartitionKnotIndex;
}

internal b_spline_knot_handle
BSplineKnotHandleFromKnotIndex(u32 KnotIndex)
{
 b_spline_knot_handle Handle = {};
 Handle.__Index = KnotIndex + 1;
 return Handle;
}

internal b_spline_knot_handle
BSplineKnotHandleFromPartitionKnotIndex(curve *Curve, u32 PartitionKnotIndex)
{
 b_spline_params BSplineParams = GetBSplineParams(Curve);
 b_spline_knot_params KnotParams = BSplineParams.KnotParams;
 u32 KnotIndex = KnotParams.Degree + PartitionKnotIndex;
 b_spline_knot_handle Handle = BSplineKnotHandleFromKnotIndex(KnotIndex);
 return Handle;
}

internal void
ControlPointAndBezierOffsetFromCurvePoint(curve_point_handle CurvePoint,
                                          control_point_handle *ControlPoint,
                                          u32 *BezierOffset)
{
 switch (CurvePoint.Type)
 {
  case CurvePoint_ControlPoint: {
   *ControlPoint = CurvePoint.Control;
   *BezierOffset = 1;
  }break;
  case CurvePoint_CubicBezierPoint: {
   u32 BezierIndex = IndexFromCubicBezierPointHandle(CurvePoint.Bezier);
   *ControlPoint = ControlPointFromCubicBezierPoint(CurvePoint.Bezier);
   *BezierOffset = BezierIndex % 3;
  }break;
 }
 Assert(*BezierOffset < 3);
}

internal void
TranslateCurvePointTo(entity_with_modify_witness *EntityWitness,
                      curve_point_handle CurvePoint,
                      v2 P,
                      translate_curve_point_flags Flags)
{
 entity *Entity = EntityWitness->Entity;
 curve *Curve = SafeGetCurve(Entity);
 
 control_point_handle ControlPoint = {};
 u32 BezierOffset = 0;
 ControlPointAndBezierOffsetFromCurvePoint(CurvePoint, &ControlPoint, &BezierOffset);
 
 u32 ControlPointIndex = 0;
 if (IndexFromControlPointHandleSafe(ControlPoint, &ControlPointIndex))
 {
  curve_points_static *Points = GetCurvePoints(Curve);
  
  if (ControlPointIndex < Points->ControlPointCount)
  {
   cubic_bezier_point *B = Points->CubicBezierPoints + ControlPointIndex;
   v2 *TranslatePoint = B->Ps + BezierOffset;
   v2 LocalP = WorldToLocalEntityPosition(Entity, P);
   
   if (BezierOffset == 1)
   {
    v2 LocalTranslation = LocalP - B->P1;
    
    B->P0 += LocalTranslation;
    B->P2 += LocalTranslation;
    
    Points->ControlPoints[ControlPointIndex] += LocalTranslation;
   }
   else
   {
    Assert(BezierOffset == 0 || BezierOffset == 2);
    v2 *Twin = B->Ps + (2 - BezierOffset);
    
    v2 DesiredTwinDirection;
    if (Flags & TranslateCurvePoint_MatchBezierTwinDirection)
    {
     DesiredTwinDirection = B->P1 - LocalP;
    }
    else
    {
     DesiredTwinDirection = *Twin - B->P1;
    }
    Normalize(&DesiredTwinDirection);
    
    v2 DesiredLengthVector;
    if (Flags & TranslateCurvePoint_MatchBezierTwinLength)
    {
     DesiredLengthVector = B->P1 - LocalP;
    }
    else
    {
     DesiredLengthVector = *Twin - B->P1;
    }
    f32 DesiredTwinLength = Norm(DesiredLengthVector);
    
    *Twin = B->P1 + DesiredTwinLength * DesiredTwinDirection;
   }
   
   *TranslatePoint = LocalP;
  }
  
  MarkEntityModified(EntityWitness);
 }
}

internal void
SelectControlPoint(curve *Curve, control_point_handle ControlPoint)
{
 if (Curve)
 {
  Curve->SelectedControlPoint = ControlPoint;
 }
}

internal void
SelectControlPointFromCurvePoint(curve *Curve, curve_point_handle CurvePoint)
{
 if (Curve)
 {
  control_point_handle ControlPoint = {};
  u32 BezierOffset = 0;
  ControlPointAndBezierOffsetFromCurvePoint(CurvePoint, &ControlPoint, &BezierOffset);
  Curve->SelectedControlPoint = ControlPoint;
 }
}

internal void
MaybeRecomputeCurveBSplineKnots(curve *Curve, b32 ForceRecompute)
{
 curve_points_static *Points = GetCurvePoints(Curve);
 u32 ControlPointCount = Points->ControlPointCount;
 curve_params *CurveParams = &Curve->Params;
 b_spline_params *RequestedBSplineParams = &CurveParams->BSpline;
 
 b_spline_knot_params FixedBSplineKnotParams = BSplineKnotParamsFromDegree(RequestedBSplineParams->KnotParams.Degree, ControlPointCount);
 Assert(FixedBSplineKnotParams.KnotCount <= MAX_B_SPLINE_KNOT_COUNT);
 b_spline_params FixedBSplineParams = {};
 FixedBSplineParams.Partition = RequestedBSplineParams->Partition;
 FixedBSplineParams.KnotParams = FixedBSplineKnotParams;
 CurveParams->BSpline = FixedBSplineParams;
 
 b_spline_params ComputedBSplineParams = Curve->ComputedBSplineParams;
 
 if (!StructsEqual(&ComputedBSplineParams, &FixedBSplineParams) || ForceRecompute)
 {
  f32 *Knots = Points->BSplineKnots;
  switch (FixedBSplineParams.Partition)
  {
   case BSplinePartition_Natural:
   case BSplinePartition_Custom: {
    if (FixedBSplineParams.Partition == BSplinePartition_Natural)
    {
     BSplineBaseKnots(FixedBSplineKnotParams, Knots);
     BSplineKnotsNaturalExtension(FixedBSplineParams.KnotParams, Knots);
    }
    else if (FixedBSplineParams.Partition == BSplinePartition_Custom)
    {
     u32 ComputedKnotCount = ComputedBSplineParams.KnotParams.KnotCount;
     u32 FixedKnotCount = FixedBSplineParams.KnotParams.KnotCount;
     f32 LastKnot = (ComputedKnotCount > 0 ? Knots[ComputedKnotCount - 1] : FixedBSplineParams.KnotParams.B);
     // NOTE(hbr): Just knots with appropriate amount of [B] knots at the end
     for (u32 KnotIndex = ComputedKnotCount;
          KnotIndex < FixedKnotCount;
          ++KnotIndex)
     {
      Knots[KnotIndex] = LastKnot;
     }
    }
    else
    {
     InvalidPath;
    }
   }break;
   
   case BSplinePartition_Periodic: {
    BSplineBaseKnots(FixedBSplineKnotParams, Knots);
    BSplineKnotsPeriodicExtension(FixedBSplineParams.KnotParams, Knots);
   }break;
   
   case BSplinePartition_Count: InvalidPath;
  }
  Curve->ComputedBSplineParams = FixedBSplineParams;
 }
}

internal void
RemoveControlPoint(entity_with_modify_witness *EntityWitness, control_point_handle Point)
{
 entity *Entity = EntityWitness->Entity;
 curve *Curve = SafeGetCurve(Entity);
 u32 Index = IndexFromControlPointHandle(Point);
 curve_points_static *Points = GetCurvePoints(Curve);
 
 if (Index < Points->ControlPointCount)
 {
#define ArrayRemoveOrdered(Array, At, Count) \
MemoryMove((Array) + (At), \
(Array) + (At) + 1, \
((Count) - (At) - 1) * SizeOf((Array)[0]))
  ArrayRemoveOrdered(Points->ControlPoints,       Index, Points->ControlPointCount);
  ArrayRemoveOrdered(Points->ControlPointWeights, Index, Points->ControlPointCount);
  ArrayRemoveOrdered(Points->CubicBezierPoints,   Index, Points->ControlPointCount);
#undef ArrayRemoveOrdered
  
  --Points->ControlPointCount;
  
  // NOTE(hbr): Maybe fix selected control point
  if (IsControlPointSelected(Curve))
  {
   u32 SelectedIndex = IndexFromControlPointHandle(Curve->SelectedControlPoint);
   if (Index == SelectedIndex)
   {
    DeselectControlPoint(Curve);
   }
   else if (Index < SelectedIndex)
   {
    control_point_handle Fixed = ControlPointHandleFromIndex(SelectedIndex - 1);
    SelectControlPoint(Curve, Fixed);
   }
  }
  
  MarkEntityModified(EntityWitness);
 }
}

internal void
DeselectControlPoint(curve *Curve)
{
 if (Curve)
 {
  Curve->SelectedControlPoint = ControlPointHandleZero();
 }
}

internal b32
IsControlPointSelected(curve *Curve)
{
 b32 Result = false;
 if (UsesControlPoints(Curve) &&
     !ControlPointHandleMatch(Curve->SelectedControlPoint, ControlPointHandleZero()))
 {
  u32 Index = IndexFromControlPointHandle(Curve->SelectedControlPoint);
  curve_points_static *Points = GetCurvePoints(Curve);
  Result = (Index < Points->ControlPointCount);
 }
 return Result;
}

internal control_point_handle
AppendControlPoint(entity_with_modify_witness *EntityWitness, v2 P)
{
 entity *Entity = EntityWitness->Entity;
 u32 InsertAt = 0;
 if (Entity->Flags & EntityFlag_CurveAppendFront)
 {
  InsertAt = 0;
 }
 else
 {
  curve *Curve = SafeGetCurve(Entity);
  curve_points_static *Points = GetCurvePoints(Curve);
  InsertAt = Points->ControlPointCount;
 }
 control_point_handle Result = InsertControlPoint(EntityWitness, MakeControlPoint(P), InsertAt);
 return Result;
}

internal void
SetControlPoint(entity_with_modify_witness *Witness, control_point_handle Handle, control_point Point)
{
 entity *Entity = Witness->Entity;
 curve *Curve = SafeGetCurve(Entity);
 
 u32 I = IndexFromControlPointHandle(Handle);
 curve_points_static *Points = GetCurvePoints(Curve);
 
 u32 N = Points->ControlPointCount;
 v2 *P = Points->ControlPoints;
 f32 *W = Points->ControlPointWeights;
 cubic_bezier_point *B = Points->CubicBezierPoints;
 
 P[I] = WorldToLocalEntityPosition(Entity, Point.P);
 W[I] = (Point.IsWeight ? Point.Weight : 1.0f);
 if (Point.IsBezier)
 {
  cubic_bezier_point Bezier = {
   WorldToLocalEntityPosition(Entity, Point.Bezier.P0),
   WorldToLocalEntityPosition(Entity, Point.Bezier.P1),
   WorldToLocalEntityPosition(Entity, Point.Bezier.P2),
  };
  B[I] = Bezier;
 }
 else
 {
  // NOTE(hbr): Cubic bezier point calculation is not really defined
  // for N<=2. At least I tried to "define" it but failed to do something
  // that was super useful. In that case, when the third point is added,
  // just recalculate all bezier points.
  if (Points->ControlPointCount == 2)
  {
   BezierCubicCalculateAllControlPoints(N+1, P, B);
  }
  else
  {
   CalculateBezierCubicPointAt(N+1, P, B, I);
  }
 }
 
 MarkEntityModified(Witness);
}

internal control_point_handle
InsertControlPoint(entity_with_modify_witness *EntityWitness, control_point Point, u32 At)
{
 control_point_handle Result = {};
 entity *Entity = EntityWitness->Entity;
 curve *Curve = SafeGetCurve(Entity);
 curve_points_static *Points = GetCurvePoints(Curve);
 
 if (Curve &&
     Points->ControlPointCount < CURVE_POINTS_STATIC_MAX_CONTROL_POINT_COUNT &&
     At <= Points->ControlPointCount)
 {
  Assert(UsesControlPoints(Curve));
  
  control_point_handle Handle = ControlPointHandleFromIndex(At);
  u32 N = Points->ControlPointCount;
  v2 *P = Points->ControlPoints;
  f32 *W = Points->ControlPointWeights;
  cubic_bezier_point *B = Points->CubicBezierPoints;
  
#define ShiftRightArray(Array, ArrayLength, At, ShiftCount) \
MemoryMove((Array) + ((At)+(ShiftCount)), \
(Array) + (At), \
((ArrayLength) - (At)) * SizeOf((Array)[0]))
  ShiftRightArray(P, N, At, 1);
  ShiftRightArray(W, N, At, 1);
  ShiftRightArray(B, N, At, 1);
#undef ShiftRightArray
  
  SetControlPoint(EntityWitness, Handle, Point);
  
  if (IsControlPointSelected(Curve))
  {
   u32 SelectedIndex = IndexFromControlPointHandle(Curve->SelectedControlPoint);
   if (SelectedIndex >= At)
   {
    control_point_handle Fixed = ControlPointHandleFromIndex(SelectedIndex + 1);
    SelectControlPoint(Curve, Fixed);
   }
  }
  
  ++Points->ControlPointCount;
  Result = Handle;
 }
 
 return Result;
}

internal void
SetCurvePoints(entity_with_modify_witness *EntityWitness, curve_points_handle NewPoints)
{
 entity *Entity = EntityWitness->Entity;
 curve *Curve = SafeGetCurve(Entity);
 curve_points_static *Points = GetCurvePoints(Curve);
 CopyCurvePoints(CurvePointsDynamicFromStatic(Points), NewPoints);
 DeselectControlPoint(Curve);
 MarkEntityModified(EntityWitness);
}

inline internal void
SetEntityName(entity *Entity, string Name)
{
 char_buffer *CharBuffer = CharBufferFromStringId(GetCtx()->StrStore, Entity->Name);
 FillCharBuffer(CharBuffer, Name);
}

internal void
InitEntityPart(entity *Entity,
               entity_type Type,
               xform2d XForm,
               string Name,
               i32 SortingLayer,
               entity_flags Flags)
{
 Entity->Type = Type;
 Entity->XForm = XForm;
 SetEntityName(Entity, Name);
 Entity->SortingLayer = SortingLayer;
 Entity->Flags = (Flags & ~EntityFlag_Selected);
}

internal void
InitEntityCurvePart(curve *Curve, curve_params CurveParams)
{
 Curve->Params = CurveParams;
}

internal void
InitEntityAsCurve(entity *Entity, string Name, curve_params CurveParams)
{
 InitEntityPart(Entity, Entity_Curve, XForm2DZero(), Name, 0, 0);
 InitEntityCurvePart(&Entity->Curve, CurveParams);
}

internal void
InitEntityImagePart(image *Image,
                    scale2d Dim,
                    u32 OriginalWidth,
                    u32 OriginalHeight,
                    string ImageFilePath,
                    render_texture_handle TextureHandle)
{
 Image->Dim = Dim;
 Image->OriginalWidth = OriginalWidth;
 Image->OriginalHeight = OriginalHeight;
 char_buffer *FilePathBuffer = CharBufferFromStringId(GetCtx()->StrStore, Image->FilePath);
 FillCharBuffer(FilePathBuffer, ImageFilePath);
 Image->TextureHandle = TextureHandle;
}

internal void
InitEntityImagePart(image *Image,
                    u32 Width, u32 Height,
                    string ImageFilePath,
                    render_texture_handle TextureHandle)
{
 scale2d Dim = Scale2D(Cast(f32)Width / Height, 1.0f);
 InitEntityImagePart(Image, Dim, Width, Height, ImageFilePath, TextureHandle);
}

internal void
InitEntityAsImage(entity *Entity,
                  v2 P,
                  u32 Width, u32 Height,
                  string ImageFilePath,
                  render_texture_handle TextureHandle)
{
 string FileName = PathLastPart(ImageFilePath);
 string FileNameNoExt = StrChopLastDot(FileName);
 InitEntityPart(Entity, Entity_Image, XForm2DFromP(P), FileNameNoExt, 0, 0);
 InitEntityImagePart(&Entity->Image, Width, Height, ImageFilePath, TextureHandle);
}

internal void
InitEntityPartFromEntity(entity *Dst, entity *Src)
{
 InitEntityPart(Dst,
                Src->Type,
                Src->XForm,
                GetEntityName(Src),
                Src->SortingLayer,
                Src->Flags);
}

internal void
InitParametricCurveField(parametric_curve_field *Dst, parametric_curve_field *Src)
{
 Dst->EquationOrDragFloatMode_Equation = Src->EquationOrDragFloatMode_Equation;
 Dst->DragValue = Src->DragValue;
 FillCharBuffer(Dst->VarName, Src->VarName);
 FillCharBuffer(Dst->Equation, Src->Equation);
}

internal void
InitParametricCurveResources(parametric_curve_resources *Dst, parametric_curve_resources *Src)
{
 InitParametricCurveField(&Dst->MinT_Var, &Src->MinT_Var);
 InitParametricCurveField(&Dst->MaxT_Var, &Src->MaxT_Var);
 InitParametricCurveField(&Dst->X_Equation, &Src->X_Equation);
 InitParametricCurveField(&Dst->Y_Equation, &Src->Y_Equation);
 
 ForEachElement(AdditionalVarIndex, Dst->AdditionalVars)
 {
  parametric_curve_field *SrcVar = Src->AdditionalVars + AdditionalVarIndex;
  if (IsParametricCurveVarActive(SrcVar))
  {
   parametric_curve_field *DstVar = AllocParametricCurveVar(Dst);
   InitParametricCurveField(DstVar, SrcVar);
  }
 }
}

internal void
InitEntityFromEntity(entity_with_modify_witness *DstWitness, entity *Src, b32 CopyBSplineCurveInCaseCustom)
{
 entity *Dst = DstWitness->Entity;
 curve *DstCurve = &Dst->Curve;
 curve *SrcCurve = &Src->Curve;
 image *DstImage = &Dst->Image;
 image *SrcImage = &Src->Image;
 
 //- init entity part
 InitEntityPart(Dst,
                Src->Type,
                Src->XForm,
                GetEntityName(Src),
                Src->SortingLayer,
                Src->Flags);
 
 //- init specific entity type part
 switch (Src->Type)
 {
  case Entity_Curve: {
   InitEntityCurvePart(DstCurve, SrcCurve->Params);
   // TODO(hbr): This is mega ad-hoc, fix in the future
   if (CopyBSplineCurveInCaseCustom)
   {
    DstCurve->ComputedBSplineParams = SrcCurve->ComputedBSplineParams;
   }
   InitParametricCurveResources(&DstCurve->ParametricResources, &SrcCurve->ParametricResources);
   DstCurve->PointTracking = SrcCurve->PointTracking;
   curve_points_static *SrcPoints = GetCurvePoints(SrcCurve);
   SetCurvePoints(DstWitness, CurvePointsHandleFromCurvePointsStatic(SrcPoints));
   SelectControlPoint(DstCurve, SrcCurve->SelectedControlPoint);
  }break;
  
  case Entity_Image: {
   DeallocTextureHandle(GetCtx()->EntityStore, DstImage->TextureHandle);
   render_texture_handle TextureHandle = CopyTextureHandle(GetCtx()->EntityStore, SrcImage->TextureHandle);
   string SrcFilePath = StringFromStringId(GetCtx()->StrStore, SrcImage->FilePath);
   InitEntityImagePart(DstImage, SrcImage->Dim, SrcImage->OriginalWidth, SrcImage->OriginalHeight, SrcFilePath, TextureHandle);
  }break;
  
  case Entity_Count: InvalidPath;
 }
}

internal void
SetControlPoint(entity_with_modify_witness *EntityWitness, control_point_handle Handle, v2 P, f32 Weight)
{
 curve_point_handle CurvePoint = CurvePointFromControlPoint(Handle);
 entity *Entity = EntityWitness->Entity;
 TranslateCurvePointTo(EntityWitness, CurvePoint, P, TranslateCurvePoint_None);
 
 if (!ControlPointHandleMatch(Handle, ControlPointHandleZero()))
 {
  curve *Curve = SafeGetCurve(Entity);
  u32 Index = IndexFromControlPointHandle(Handle);
  curve_points_static *Points = GetCurvePoints(Curve);
  Points->ControlPointWeights[Index] = Weight;
  MarkEntityModified(EntityWitness);
 }
}

internal curve_points_modify_handle
BeginModifyCurvePoints(entity_with_modify_witness *EntityWitness, u32 RequestedPointCount, modify_curve_points_static_which_points Which)
{
 curve_points_modify_handle Result = {};
 
 entity *Entity = EntityWitness->Entity;
 curve *Curve = SafeGetCurve(Entity);
 curve_points_static *Points = GetCurvePoints(Curve);
 Result.Curve = Curve;
 u32 ActualPointCount = Min(RequestedPointCount, CURVE_POINTS_STATIC_MAX_CONTROL_POINT_COUNT);
 Result.PointCount = ActualPointCount;
 Result.ControlPoints = Points->ControlPoints;
 Result.Weights = Points->ControlPointWeights;
 Result.CubicBeziers = Points->CubicBezierPoints;
 Result.BSplineKnots = Points->BSplineKnots;
 Result.Which = Which;
 // NOTE(hbr): Assume (which is very reasonable) that the curve is always changing here
 MarkEntityModified(EntityWitness);
 
 return Result;
}

internal void
EndModifyCurvePoints(curve_points_modify_handle Handle)
{
 curve *Curve = Handle.Curve;
 curve_params *CurveParams = &Curve->Params;
 curve_points_static *Points = GetCurvePoints(Curve);
 Points->ControlPointCount = Handle.PointCount;
 if (Handle.Which <= ModifyCurvePointsWhichPoints_ControlPointsOnly)
 {
  for (u32 PointIndex = 0;
       PointIndex < Points->ControlPointCount;
       ++PointIndex)
  {
   Points->ControlPointWeights[PointIndex] = 1.0f;
  }
 }
 if (Handle.Which <= ModifyCurvePointsWhichPoints_ControlPointsAndWeights)
 {
  BezierCubicCalculateAllControlPoints(Points->ControlPointCount,
                                       Points->ControlPoints,
                                       Points->CubicBezierPoints);
 }
 if (Handle.Which <= ModifyCurvePointsWhichPoints_ControlPointsAndWeightsAndCubicBeziers)
 {
  MaybeRecomputeCurveBSplineKnots(Curve, true);
 }
}

inline internal v2
ProjectThroughXForm(xform2d XForm, v2 P)
{
 v2 Q = P;
 Q = Hadamard(Q, XForm.Scale.V);
 Q = RotateAround(Q, V2(0.0f, 0.0f), XForm.Rotation);
 Q = Q + XForm.P;
 return Q;
}

inline internal v2
UnprojectThroughXForm(xform2d XForm, v2 P)
{
 v2 Q = P;
 Q = Q - XForm.P;
 Q = RotateAround(Q, V2(0, 0), Rotation2DInverse(XForm.Rotation));
 Q = Hadamard(Q, V2(1.0f / XForm.Scale.V.X, 1.0f / XForm.Scale.V.Y));
 return Q;
}

// TODO(hbr): Consider using ModelTransform instead (might be a little slower but probably doesn't matter)
internal v2
WorldToLocalEntityPosition(entity *Entity, v2 P)
{
 v2 Result = UnprojectThroughXForm(Entity->XForm, P);
 return Result;
}

internal v2
LocalToWorldEntityPosition(entity *Entity, v2 P)
{
 v2 Result = ProjectThroughXForm(Entity->XForm, P);
 return Result;
}

internal void
SetTrackingPointFraction(entity_with_modify_witness *EntityWitness, f32 Fraction)
{
 entity *Entity = EntityWitness->Entity;
 curve *Curve = SafeGetCurve(Entity);
 point_tracking_along_curve_state *Tracking = &Curve->PointTracking;
 Tracking->Fraction = Clamp01(Fraction);
 MarkEntityModified(EntityWitness);
}

internal void
SetBSplineKnotPoint(entity_with_modify_witness *EntityWitness,
                    b_spline_knot_handle Knot,
                    f32 KnotFraction)
{
 entity *Entity = EntityWitness->Entity;
 curve *Curve = SafeGetCurve(Entity);
 curve_points_static *Points = GetCurvePoints(Curve);
 b_spline_knot_params KnotParams = GetBSplineParams(Curve).KnotParams;
 f32 *Knots = Points->BSplineKnots;
 u32 KnotIndex = KnotIndexFromBSplineKnotHandle(Knot);
 b32 Moveable = ((KnotIndex >= KnotParams.Degree + 1) &&
                 (KnotIndex + 1 < KnotParams.Degree + KnotParams.PartitionSize));
 Assert(Moveable);
 if (Moveable)
 {
  for (u32 Index = KnotIndex;
       Index > 0;
       --Index)
  {
   Knots[Index - 1] = Min(KnotFraction, Knots[Index - 1]);
  }
  for (u32 Index = KnotIndex + 1;
       Index < KnotParams.KnotCount;
       ++Index)
  {
   Knots[Index] = Max(KnotFraction, Knots[Index]);
  }
  Knots[KnotIndex] = KnotFraction;
  MarkEntityModified(EntityWitness);
 }
}

// TODO(hbr): Try to get rid of this function entirely
internal b32
IsCurveLooped(curve *Curve)
{
 b32 IsLooped = (Curve->Params.Type == Curve_CubicSpline &&
                 Curve->Params.CubicSpline == CubicSpline_Periodic);
 return IsLooped;
}

struct points_soa
{
 f32 *Xs;
 f32 *Ys;
};
internal points_soa
SplitPointsIntoComponents(arena *Arena, v2 *Points, u32 PointCount)
{
 points_soa Result = {};
 Result.Xs = PushArrayNonZero(Arena, PointCount, f32);
 Result.Ys = PushArrayNonZero(Arena, PointCount, f32);
 for (u32 I = 0; I < PointCount; ++I)
 {
  Result.Xs[I] = Points[I].X;
  Result.Ys[I] = Points[I].Y;
 }
 
 return Result;
}

internal void
CalcNormalBezierLinePoints(v2 *ControlPoints,
                           u32 ControlPointCount,
                           u32 OutputLinePointCount,
                           v2 *OutputLinePoints)
{
 f32 T = 0.0f;
 f32 Delta = 1.0f / (OutputLinePointCount - 1);
 for (u32 OutputIndex = 0;
      OutputIndex < OutputLinePointCount;
      ++OutputIndex)
 {
  
  OutputLinePoints[OutputIndex] = BezierCurveEvaluate(T,
                                                      ControlPoints,
                                                      ControlPointCount);
  
  T += Delta;
 }
}

struct cubic_bezier_curve_segment
{
 u32 PointCount;
 v2 *Points;
};
internal cubic_bezier_curve_segment
GetCubicBezierCurveSegment(cubic_bezier_point *BezierPoints, u32 PointCount, u32 SegmentIndex)
{
 cubic_bezier_curve_segment Result = {};
 if (SegmentIndex < PointCount)
 {
  Result.PointCount = (SegmentIndex < PointCount - 1 ? 4 : 1);
  Result.Points = &BezierPoints[SegmentIndex].P1;
 }
 
 return Result;
}

internal void
CalcBarycentricPolynomial(v2 *Controls, u32 PointCount, point_spacing PointSpacing, f32 *Ti,
                          u32 SampleCount, v2 *OutSamples,
                          f32 Min_T, f32 Max_T, u32 SamplesPerControlPoint)
{
 if (PointCount > 0)
 {
  temp_arena Temp = TempArena(0);
  
  f32 *Omega = PushArrayNonZero(Temp.Arena, PointCount, f32);
  switch (PointSpacing)
  {
   case PointSpacing_Equidistant: {
    // NOTE(hbr): Equidistant version doesn't seem to work properly (precision problems)
    BarycentricOmega(Omega, Ti, PointCount);
   } break;
   case PointSpacing_Chebychev: {
    BarycentricOmegaChebychev(Omega, PointCount);
   } break;
   
   case PointSpacing_Count: InvalidPath;
  }
  
  points_soa SOA = SplitPointsIntoComponents(Temp.Arena, Controls, PointCount);
  
  // TODO(hbr): This is kinda of awful, refactor this
  v2 *Out = OutSamples;
  for (u32 PointIndex = 1;
       PointIndex < PointCount;
       ++PointIndex)
  {
   f32 Min_TT = Ti[PointIndex - 1];
   f32 Max_TT = Ti[PointIndex - 0];
   f32 Delta_TT = (Max_TT - Min_TT) / SamplesPerControlPoint;
   f32 TT = Min_TT;
   for (u32 SampleIndex = 0;
        SampleIndex < SamplesPerControlPoint;
        ++SampleIndex)
   {
    f32 X = BarycentricEvaluate(TT, Omega, Ti, SOA.Xs, PointCount);
    f32 Y = BarycentricEvaluate(TT, Omega, Ti, SOA.Ys, PointCount);
    *Out++ = V2(X, Y);
    TT += Delta_TT;
   }
  }
  *Out++ = Controls[PointCount - 1];
  
  EndTemp(Temp);
 }
}

internal void
CalcNewtonPolynomial(v2 *Controls, u32 PointCount, f32 *Ti,
                     u32 SampleCount, v2 *OutSamples,
                     f32 Min_T, f32 Max_T, u32 SamplesPerControlPoint)
{
 if (PointCount > 0)
 {
  temp_arena Temp = TempArena(0);
  
  points_soa SOA = SplitPointsIntoComponents(Temp.Arena, Controls, PointCount);
  
  f32 *Beta_X = PushArrayNonZero(Temp.Arena, PointCount, f32);
  f32 *Beta_Y = PushArrayNonZero(Temp.Arena, PointCount, f32);
  NewtonBetaFast(Beta_X, Ti, SOA.Xs, PointCount);
  NewtonBetaFast(Beta_Y, Ti, SOA.Ys, PointCount);
  
  // TODO(hbr): This is kinda of awful, refactor this
  v2 *Out = OutSamples;
  for (u32 PointIndex = 1;
       PointIndex < PointCount;
       ++PointIndex)
  {
   f32 Min_TT = Ti[PointIndex - 1];
   f32 Max_TT = Ti[PointIndex - 0];
   f32 Delta_TT = (Max_TT - Min_TT) / SamplesPerControlPoint;
   f32 TT = Min_TT;
   for (u32 SampleIndex = 0;
        SampleIndex < SamplesPerControlPoint;
        ++SampleIndex)
   {
    f32 X = NewtonEvaluate(TT, Beta_X, Ti, PointCount);
    f32 Y = NewtonEvaluate(TT, Beta_Y, Ti, PointCount);
    *Out++ = V2(X, Y);
    TT += Delta_TT;
   }
  }
  *Out++ = Controls[PointCount - 1];
  
  EndTemp(Temp);
 }
}

internal void
CalcPolynomial(v2 *Controls, u32 PointCount,
               polynomial_interpolation_params Polynomial,
               u32 SampleCount, v2 *OutSamples,
               u32 SamplesPerControlPoint)
{
 temp_arena Temp = TempArena(0);
 
 point_spacing PointSpacing = Polynomial.PointSpacing;
 
 f32 *Ti = PushArrayNonZero(Temp.Arena, PointCount, f32);
 switch (PointSpacing)
 {
  case PointSpacing_Equidistant: { EquidistantPoints(Ti, PointCount, 0.0f, 1.0f); } break;
  case PointSpacing_Chebychev:   { ChebyshevPoints(Ti, PointCount);   } break;
  case PointSpacing_Count: InvalidPath;
 }
 
 f32 Min_T = 0;
 f32 Max_T = 0;
 if (PointCount > 0)
 {
  Min_T = Ti[0];
  Max_T = Ti[PointCount - 1];
 }
 
 switch (Polynomial.Type)
 {
  case PolynomialInterpolation_Barycentric: {CalcBarycentricPolynomial(Controls, PointCount, PointSpacing, Ti, SampleCount, OutSamples, Min_T, Max_T, SamplesPerControlPoint);}break;
  case PolynomialInterpolation_Newton:      {CalcNewtonPolynomial(Controls, PointCount, Ti, SampleCount, OutSamples, Min_T, Max_T, SamplesPerControlPoint);}break;
  case PolynomialInterpolation_Count: InvalidPath;
 }
 
 EndTemp(Temp);
}

internal void
CalcCubicSpline_Scalar(f32 *Xs,
                       f32 *Ys,
                       u32 PointCount,
                       f32 *Ti,
                       f32 *Mx,
                       f32 *My,
                       u32 SampleCount,
                       f32 *Ts,
                       v2 *OutSamples)
{
 ForEachIndex(SampleIndex, SampleCount)
 {
  f32 T = Ts[SampleIndex];
  f32 X = CubicSplineEvaluateScalar(T, Mx, Ti, Xs, PointCount);
  f32 Y = CubicSplineEvaluateScalar(T, My, Ti, Ys, PointCount);
  OutSamples[SampleIndex] = V2(X, Y);
 }
}

internal void
CalcCubicSpline_ScalarWithBinarySearch(f32 *Xs,
                                       f32 *Ys,
                                       u32 PointCount,
                                       f32 *Ti,
                                       f32 *Mx,
                                       f32 *My,
                                       u32 SampleCount,
                                       f32 *Ts,
                                       v2 *OutSamples)
{
 ForEachIndex(SampleIndex, SampleCount)
 {
  f32 T = Ts[SampleIndex];
  f32 X = CubicSplineEvaluateScalarWithBinarySearch(T, Mx, Ti, Xs, PointCount);
  f32 Y = CubicSplineEvaluateScalarWithBinarySearch(T, My, Ti, Ys, PointCount);
  OutSamples[SampleIndex] = V2(X, Y);
 }
}

internal void
CalcCubicSpline_ScalarWithConstantSearch(f32 *Xs,
                                         f32 *Ys,
                                         u32 PointCount,
                                         f32 *Ti,
                                         f32 *Mx,
                                         f32 *My,
                                         u32 SampleCount,
                                         f32 *Ts,
                                         v2 *OutSamples)
{
 cubic_spline_evaluate_iterator Itx = {}, Ity = {};
 ForEachIndex(SampleIndex, SampleCount)
 {
  f32 T = Ts[SampleIndex];
  f32 X = CubicSplineEvaluateScalarWithConstantSearch(T, Mx, Ti, Xs, PointCount, &Itx);
  f32 Y = CubicSplineEvaluateScalarWithConstantSearch(T, My, Ti, Ys, PointCount, &Ity);
  OutSamples[SampleIndex] = V2(X, Y);
 }
}

struct calc_cubic_spline_work
{
 f32 *Xs;
 f32 *Ys;
 u32 PointCount;
 f32 *Ti;
 f32 *Mx;
 f32 *My;
 u32 SampleCount;
 f32 *Ts;
 v2 *OutSamples;
};

internal void
CalcCubicSpline_Scalar_Work(void *UserData)
{
 calc_cubic_spline_work *Work = Cast(calc_cubic_spline_work *)UserData;
 CalcCubicSpline_Scalar(Work->Xs,
                        Work->Ys,
                        Work->PointCount,
                        Work->Ti,
                        Work->Mx,
                        Work->My,
                        Work->SampleCount,
                        Work->Ts,
                        Work->OutSamples);
}

internal void
CalcCubicSpline_ScalarWithBinarySearch_Work(void *UserData)
{
 calc_cubic_spline_work *Work = Cast(calc_cubic_spline_work *)UserData;
 CalcCubicSpline_ScalarWithBinarySearch(Work->Xs,
                                        Work->Ys,
                                        Work->PointCount,
                                        Work->Ti,
                                        Work->Mx,
                                        Work->My,
                                        Work->SampleCount,
                                        Work->Ts,
                                        Work->OutSamples);
}

struct work_queue_blocks
{
 u32 BlockCount;
 u32 BlockSize;
};
internal work_queue_blocks
WorkQueueCalculateBlocks(work_queue *WorkQueue, u32 ComputeCount, u32 RequestBlockSize)
{
 u32 RequestBlockCount = (ComputeCount + RequestBlockSize - 1) / RequestBlockSize;
 u32 FreeEntries = Platform.WorkQueueFreeEntryCount(WorkQueue);
 u32 ActualBlockCount = Min(RequestBlockCount, FreeEntries);
 u32 ActualBlockSize = (ComputeCount + ActualBlockCount - 1) / ActualBlockCount;
 
 work_queue_blocks Result = {};
 Result.BlockCount = ActualBlockCount;
 Result.BlockSize = ActualBlockSize;
 Assert(ActualBlockCount * ActualBlockSize >= ComputeCount);
 Assert((ActualBlockCount - 1) * ActualBlockSize < ComputeCount || ActualBlockCount == 0);
 
 return Result;
}

internal void
CalcCubicSpline_MultiThreaded(f32 *Xs,
                              f32 *Ys,
                              u32 PointCount,
                              f32 *Ti,
                              f32 *Mx,
                              f32 *My,
                              u32 SampleCount,
                              f32 *Ts,
                              v2 *OutSamples,
                              work_queue_func *EvalWorkFunc)
{
 ProfileFunctionBegin();
 
 temp_arena Temp = TempArena(0);
 work_queue *WorkQueue = GetCtx()->HighPriorityQueue;
 
 // NOTE(hbr): 256 selected experimentally
 work_queue_blocks Blocks = WorkQueueCalculateBlocks(WorkQueue, SampleCount, 256);
 u32 BlockCount = Blocks.BlockCount;
 u32 BlockSize = Blocks.BlockSize;
 u32 SamplesLeft = SampleCount;
 f32 *TsAt = Ts;
 v2 *OutSamplesAt = OutSamples;
 
 calc_cubic_spline_work *Works = PushArray(Temp.Arena, BlockCount, calc_cubic_spline_work);
 
 ForEachIndex(BlockIndex, BlockCount)
 {
  u32 BlockSampleCount = Min(SamplesLeft, BlockSize);
  
  calc_cubic_spline_work *Work = Works + BlockIndex;
  Work->Xs = Xs;
  Work->Ys = Ys;
  Work->PointCount = PointCount;
  Work->Ti = Ti;
  Work->Mx = Mx;
  Work->My = My;
  Work->SampleCount = BlockSampleCount;
  Work->Ts = TsAt;
  Work->OutSamples = OutSamplesAt;
  
  Platform.WorkQueueAddEntry(WorkQueue, EvalWorkFunc, Work);
  
  TsAt += BlockSampleCount;
  OutSamplesAt += BlockSampleCount;
  SamplesLeft -= BlockSampleCount;
 }
 
 Platform.WorkQueueCompleteAllWork(WorkQueue);
 
 EndTemp(Temp);
 
 ProfileEnd();
}

internal void
CalcCubicSpline(v2 *Controls,
                u32 PointCount,
                cubic_spline_type Spline,
                u32 SampleCount,
                v2 *OutSamples)
{
 ProfileFunctionBegin();
 
 if (PointCount > 0)
 {
  temp_arena Temp = TempArena(0);
  
  if (Spline == CubicSpline_Periodic)
  {
   v2 *OriginalControls = Controls;
   
   Controls = PushArrayNonZero(Temp.Arena, PointCount + 1, v2);
   ArrayCopy(Controls, OriginalControls, PointCount);
   Controls[PointCount] = OriginalControls[0];
   
   ++PointCount;
  }
  
  f32 *Ti = PushArrayNonZero(Temp.Arena, PointCount, f32);
  EquidistantPoints(Ti, PointCount, 0.0f, 1.0f);
  
  points_soa SOA = SplitPointsIntoComponents(Temp.Arena, Controls, PointCount);
  
  f32 *Mx = PushArrayNonZero(Temp.Arena, PointCount, f32);
  f32 *My = PushArrayNonZero(Temp.Arena, PointCount, f32);
  switch (Spline)
  {
   case CubicSpline_Natural: {
    CubicSplineNaturalM(Mx, Ti, SOA.Xs, PointCount);
    CubicSplineNaturalM(My, Ti, SOA.Ys, PointCount);
   } break;
   
   case CubicSpline_Periodic: {
    CubicSplinePeriodicM(Mx, Ti, SOA.Xs, PointCount);
    CubicSplinePeriodicM(My, Ti, SOA.Ys, PointCount);
   } break;
   
   case CubicSpline_Count: InvalidPath;
  }
  
  f32 T = Ti[0];
  f32 Delta_T = (Ti[PointCount - 1] - Ti[0]) / (SampleCount - 1);
  f32 *Ts = PushArrayNonZero(Temp.Arena, SampleCount, f32);
  ForEachIndex(SampleIndex, SampleCount)
  {
   Ts[SampleIndex] = T;
   T += Delta_T;
  }
  
  ProfileBlock("CalcCubicSpline - Samples Block")
  {
   switch (DEBUG_Vars->CubicSpline_EvalMethod)
   {
    case CubicSpline_Eval_Scalar: {
     CalcCubicSpline_Scalar(SOA.Xs, SOA.Ys, PointCount, Ti, Mx, My, SampleCount, Ts, OutSamples);
    }break;
    
    case CubicSpline_Eval_ScalarWithBinarySearch: {
     CalcCubicSpline_ScalarWithBinarySearch(SOA.Xs, SOA.Ys, PointCount, Ti, Mx, My, SampleCount, Ts, OutSamples);
    }break;
    
    case CubicSpline_Eval_ScalarWithConstantSearch: {
     CalcCubicSpline_ScalarWithConstantSearch(SOA.Xs, SOA.Ys, PointCount, Ti, Mx, My, SampleCount, Ts, OutSamples);
    }break;
    
    case CubicSpline_Eval_Scalar_MultiThreaded: {
     CalcCubicSpline_MultiThreaded(SOA.Xs, SOA.Ys, PointCount, Ti, Mx, My, SampleCount, Ts, OutSamples, CalcCubicSpline_Scalar_Work);
    }break;
    
    case CubicSpline_Eval_ScalarWithBinarySearch_MultiThreaded: {
     CalcCubicSpline_MultiThreaded(SOA.Xs, SOA.Ys, PointCount, Ti, Mx, My, SampleCount, Ts, OutSamples, CalcCubicSpline_ScalarWithBinarySearch_Work);
    }break;
    
    case CubicSpline_Eval_Count: InvalidPath;
   }
  }
  
  EndTemp(Temp);
 }
 
 ProfileEnd();
}

internal void
CalcBezierRational_Scalar(v2 *Controls,
                          f32 *Weights,
                          u32 PointCount,
                          u32 SampleCount,
                          f32 *Ts,
                          v2 *OutSamples)
{
 ForEachIndex(SampleIndex, SampleCount)
 {
  f32 T = Ts[SampleIndex];
  OutSamples[SampleIndex] = BezierCurveRationalEvaluateScalar(T, Controls, Weights, PointCount);
 }
}

internal void
CalcBezierRational_SSE(v2 *Controls,
                       f32 *Weights,
                       u32 PointCount,
                       u32 SampleCount,
                       f32 *Ts,
                       v2 *OutSamples)
{
 u32 Blocks = (SampleCount + 3) / 4;
 u32 I = 0;
 while (Blocks--)
 {
  f32 T4[4] = {};
  for (u32 J = 0; J < 4; ++J)
  {
   if (I + J < SampleCount)
   {
    T4[J] = Ts[I + J];
   }
  }
  v2 Out4[4] = {};
  
  BezierCurveRationalEvaluateSSE(T4,
                                 Controls,
                                 Weights,
                                 PointCount,
                                 Out4);
  
  for (u32 J = 0; J < 4; ++J)
  {
   if (I + J < SampleCount)
   {
    OutSamples[I + J] = Out4[J];
   }
  }
  
  I += 4;
 }
}

internal void
CalcBezierRational_AVX2(v2 *Controls,
                        f32 *Weights,
                        u32 PointCount,
                        u32 SampleCount,
                        f32 *Ts,
                        v2 *OutSamples)
{
 u32 Blocks = (SampleCount + 7) / 8;
 u32 I = 0;
 while (Blocks--)
 {
  f32 T8[8] = {};
  for (u32 J = 0; J < 8; ++J)
  {
   if (I + J < SampleCount)
   {
    T8[J] = Ts[I + J];
   }
  }
  v2 Out8[8] = {};
  
  BezierCurveRationalEvaluateAVX2(T8,
                                  Controls,
                                  Weights,
                                  PointCount,
                                  Out8);
  
  for (u32 J = 0; J < 8; ++J)
  {
   if (I + J < SampleCount)
   {
    OutSamples[I + J] = Out8[J];
   }
  }
  
  I += 8;
 }
}

internal void
CalcBezierRational_AVX512(v2 *Controls,
                          f32 *Weights,
                          u32 PointCount,
                          u32 SampleCount,
                          f32 *Ts,
                          v2 *OutSamples)
{
 u32 Blocks = (SampleCount + 15) / 16;
 u32 I = 0;
 while (Blocks--)
 {
  f32 T16[16] = {};
  for (u32 J = 0; J < 16; ++J)
  {
   if (I + J < SampleCount)
   {
    T16[J] = Ts[I + J];
   }
  }
  v2 Out16[16] = {};
  
  BezierCurveRational_Evaluate_AVX512(T16,
                                      Controls,
                                      Weights,
                                      PointCount,
                                      Out16);
  
  for (u32 J = 0; J < 16; ++J)
  {
   if (I + J < SampleCount)
   {
    OutSamples[I + J] = Out16[J];
   }
  }
  
  I += 16;
 }
}

struct calc_bezier_rational_work
{
 v2 *Controls;
 f32 *Weights;
 u32 PointCount;
 u32 SampleCount;
 f32 *Ts;
 v2 *OutSamples;
};

internal void
CalcBezierRational_Scalar_Work(void *UserData)
{
 calc_bezier_rational_work *Work = Cast(calc_bezier_rational_work *)UserData;
 CalcBezierRational_Scalar(Work->Controls,
                           Work->Weights,
                           Work->PointCount,
                           Work->SampleCount,
                           Work->Ts,
                           Work->OutSamples);
}

internal void
CalcBezierRational_SSE_Work(void *UserData)
{
 calc_bezier_rational_work *Work = Cast(calc_bezier_rational_work *)UserData;
 CalcBezierRational_SSE(Work->Controls,
                        Work->Weights,
                        Work->PointCount,
                        Work->SampleCount,
                        Work->Ts,
                        Work->OutSamples);
}

internal void
CalcBezierRational_AVX2_Work(void *UserData)
{
 calc_bezier_rational_work *Work = Cast(calc_bezier_rational_work *)UserData;
 CalcBezierRational_AVX2(Work->Controls,
                         Work->Weights,
                         Work->PointCount,
                         Work->SampleCount,
                         Work->Ts,
                         Work->OutSamples);
}

internal void
CalcBezierRational_AVX512_Work(void *UserData)
{
 calc_bezier_rational_work *Work = Cast(calc_bezier_rational_work *)UserData;
 CalcBezierRational_AVX512(Work->Controls,
                           Work->Weights,
                           Work->PointCount,
                           Work->SampleCount,
                           Work->Ts,
                           Work->OutSamples);
}

internal void
CalcBezierRational_MultiThreaded(v2 *Controls,
                                 f32 *Weights,
                                 u32 PointCount,
                                 u32 SampleCount,
                                 f32 *Ts,
                                 v2 *OutSamples,
                                 work_queue_func *EvalWorkFunc)
{
 temp_arena Temp = TempArena(0);
 work_queue *WorkQueue = GetCtx()->HighPriorityQueue;
 
 // NOTE(hbr): BlockSize found experimentally
 work_queue_blocks Blocks = WorkQueueCalculateBlocks(WorkQueue, SampleCount, 200);
 u32 BlockCount = Blocks.BlockCount;
 u32 BlockSize = Blocks.BlockSize;
 u32 SamplesLeft = SampleCount;
 f32 *TsAt = Ts;
 v2 *OutSamplesAt = OutSamples;
 
 calc_bezier_rational_work *Works = PushArray(Temp.Arena, BlockCount, calc_bezier_rational_work);
 
 ForEachIndex(BlockIndex, BlockCount)
 {
  u32 BlockSampleCount = Min(SamplesLeft, BlockSize);
  
  calc_bezier_rational_work *Work = Works + BlockIndex;
  Work->Controls = Controls;
  Work->Weights = Weights;
  Work->PointCount = PointCount;
  Work->SampleCount = BlockSampleCount;
  Work->Ts = TsAt;
  Work->OutSamples = OutSamplesAt;
  
  Platform.WorkQueueAddEntry(WorkQueue, EvalWorkFunc, Work);
  
  TsAt += BlockSampleCount;
  OutSamplesAt += BlockSampleCount;
  SamplesLeft -= BlockSampleCount;
 }
 
 Assert(SamplesLeft == 0);
 
 Platform.WorkQueueCompleteAllWork(WorkQueue);
 
 EndTemp(Temp);
}

internal void
CalcBezierRational(v2 *Controls,
                   f32 *Weights,
                   u32 PointCount,
                   u32 SampleCount,
                   v2 *OutSamples)
{
 ProfileFunctionBegin();
 
 temp_arena Temp = TempArena(0);
 
 f32 T = 0.0f;
 // NOTE(hbr): We assume the interval is always [0,1]
 f32 Delta_T = 1.0f / (SampleCount - 1);
 f32 *Ts = PushArrayNonZero(Temp.Arena, SampleCount, f32);
 ForEachIndex(SampleIndex, SampleCount)
 {
  Ts[SampleIndex] = T;
  T += Delta_T;
 }
 
 switch (DEBUG_Vars->Bezier_EvalMethod)
 {
  case Bezier_Eval_Scalar: {
   CalcBezierRational_Scalar(Controls, Weights, PointCount, SampleCount, Ts, OutSamples);
  }break;
  
  case Bezier_Eval_SSE: {
   CalcBezierRational_SSE(Controls, Weights, PointCount, SampleCount, Ts, OutSamples);
  }break;
  
  case Bezier_Eval_AVX2: {
   CalcBezierRational_AVX2(Controls, Weights, PointCount, SampleCount, Ts, OutSamples);
  }break;
  
  case Bezier_Eval_AVX512: {
   CalcBezierRational_AVX512(Controls, Weights, PointCount, SampleCount, Ts, OutSamples);
  }break;
  
  case Bezier_Eval_SSE_MultiThreaded: {
   CalcBezierRational_MultiThreaded(Controls, Weights, PointCount, SampleCount, Ts, OutSamples, CalcBezierRational_SSE_Work);
  }break;
  
  case Bezier_Eval_AVX2_MultiThreaded: {
   CalcBezierRational_MultiThreaded(Controls, Weights, PointCount, SampleCount, Ts, OutSamples, CalcBezierRational_AVX2_Work);
  }break;
  
  case Bezier_Eval_AVX512_MultiThreaded: {
   CalcBezierRational_MultiThreaded(Controls, Weights, PointCount, SampleCount, Ts, OutSamples, CalcBezierRational_AVX512_Work);
  }break;
  
  case Bezier_Eval_Adaptive_MultiThreaded: {
   work_queue_func *EvalFunc = 0;
   
   instruction_set_flags Flags = Platform.InstructionSetSupport();
   if (0) {}
   else if (Flags & InstructionSet_AVX512) EvalFunc = CalcBezierRational_AVX512_Work;
   else if (Flags & InstructionSet_AVX2) EvalFunc = CalcBezierRational_AVX2_Work;
   else if (Flags & InstructionSet_SSE) EvalFunc = CalcBezierRational_SSE_Work;
   else EvalFunc = CalcBezierRational_Scalar_Work;
   
   CalcBezierRational_MultiThreaded(Controls, Weights, PointCount, SampleCount, Ts, OutSamples, EvalFunc);
  }break;
  
  case Bezier_Eval_Count: InvalidPath;
 }
 
 EndTemp(Temp);
 
 ProfileEnd();
}

internal void
CalcBezierCubicSpline(cubic_bezier_point *Beziers, u32 PointCount,
                      u32 SampleCount, v2 *OutSamples)
{
 if (PointCount > 0)
 {
  f32 T = 0.0f;
  f32 Delta_T = 1.0f / (SampleCount - 1);
  
  for (u32 SampleIndex = 0;
       SampleIndex < SampleCount;
       ++SampleIndex)
  {
   f32 Expanded_T = (PointCount - 1) * T;
   u32 SegmentIndex = Cast(u32)Expanded_T;
   f32 Segment_T = Expanded_T - SegmentIndex;
   
   Assert(SegmentIndex < PointCount);
   Assert(0.0f <= Segment_T && Segment_T <= 1.0f);
   
   cubic_bezier_curve_segment Segment = GetCubicBezierCurveSegment(Beziers, PointCount, SegmentIndex);
   OutSamples[SampleIndex] = BezierCurveEvaluate(Segment_T, Segment.Points, Segment.PointCount);
   
   T += Delta_T;
  }
 }
}

inline internal u32
PartitionKnotIndexToKnotIndex(u32 PartitionKnotIndex, b_spline_knot_params KnotParams)
{
 Assert(PartitionKnotIndex < KnotParams.PartitionSize);
 u32 Result = PartitionKnotIndex + KnotParams.Degree;
 return Result;
}

internal void
CalcNURBS_ScalarUsingBSplineEvaluate(v2 *Controls,
                                     f32 *Weights,
                                     b_spline_knot_params KnotParams,
                                     f32 *Knots,
                                     u32 SampleCount,
                                     f32 *Ts,
                                     v2 *OutSamples)
{
 ProfileFunctionBegin();
 
 for (u32 SampleIndex = 0;
      SampleIndex < SampleCount;
      ++SampleIndex)
 {
  f32 T = Ts[SampleIndex];
  OutSamples[SampleIndex] = BSplineEvaluate(T, Controls, KnotParams, Knots);
 }
 
 ProfileEnd();
}

internal void
CalcNURBS_Scalar(v2 *Controls,
                 f32 *Weights,
                 b_spline_knot_params KnotParams,
                 f32 *Knots,
                 u32 SampleCount,
                 f32 *Ts,
                 v2 *OutSamples)
{
 for (u32 SampleIndex = 0;
      SampleIndex < SampleCount;
      ++SampleIndex)
 {
  f32 T = Ts[SampleIndex];
  OutSamples[SampleIndex] = NURBS_EvaluateScalar(T, Controls, Weights, KnotParams, Knots);
 }
}


internal void
CalcNURBS_ScalarButUsingSSE(v2 *Controls,
                            f32 *Weights,
                            b_spline_knot_params KnotParams,
                            f32 *Knots,
                            u32 SampleCount,
                            f32 *Ts,
                            v2 *OutSamples)
{
 for (u32 SampleIndex = 0;
      SampleIndex < SampleCount;
      ++SampleIndex)
 {
  f32 T = Ts[SampleIndex];
  f32 T4[4] = {};
  T4[0] = T;
  v2 Out4[4] = {};
  
  NURBS_EvaluateSSE(Controls,
                    Weights,
                    KnotParams.PartitionSize - 1,
                    KnotParams.Degree,
                    Knots,
                    T4,
                    Out4);
  
  OutSamples[SampleIndex] = Out4[0];
 }
}

internal void
CalcNURBS_SSE(v2 *Controls,
              f32 *Weights,
              b_spline_knot_params KnotParams,
              f32 *Knots,
              u32 SampleCount,
              f32 *Ts,
              v2 *OutSamples)
{
 u32 Blocks = (SampleCount + 3) / 4;
 u32 I = 0;
 while (Blocks--)
 {
  f32 T4[4] = {};
  for (u32 J = 0; J < 4; ++J)
  {
   if (I + J < SampleCount)
   {
    T4[J] = Ts[I + J];
   }
  }
  v2 Out4[4] = {};
  
  NURBS_EvaluateSSE(Controls,
                    Weights,
                    KnotParams.PartitionSize - 1,
                    KnotParams.Degree,
                    Knots,
                    T4,
                    Out4);
  
  for (u32 J = 0; J < 4; ++J)
  {
   if (I + J < SampleCount)
   {
    OutSamples[I + J] = Out4[J];
   }
  }
  
  I += 4;
 }
}

internal void
CalcNURBS_AVX2(v2 *Controls,
               f32 *Weights,
               b_spline_knot_params KnotParams,
               f32 *Knots,
               u32 SampleCount,
               f32 *Ts,
               v2 *OutSamples)
{
 u32 Blocks = (SampleCount + 7) / 8;
 u32 I = 0;
 while (Blocks--)
 {
  f32 T8[8] = {};
  for (u32 J = 0; J < 8; ++J)
  {
   if (I + J < SampleCount)
   {
    T8[J] = Ts[I + J];
   }
  }
  v2 Out8[8] = {};
  
  NURBS_EvaluateAVX2(Controls,
                     Weights,
                     KnotParams.PartitionSize - 1,
                     KnotParams.Degree,
                     Knots,
                     T8,
                     Out8);
  
  for (u32 J = 0; J < 8; ++J)
  {
   if (I + J < SampleCount)
   {
    OutSamples[I + J] = Out8[J];
   }
  }
  
  I += 8;
 }
}

struct calc_nurbs_work
{
 v2 *Controls;
 f32 *Weights;
 b_spline_knot_params KnotParams;
 f32 *Knots;
 u32 SampleCount;
 f32 *Ts;
 v2 *OutSamples;
};

internal void
CalcNURBS_Scalar_Work(void *UserData)
{
 calc_nurbs_work *Work = Cast(calc_nurbs_work *)UserData;
 CalcNURBS_Scalar(Work->Controls,
                  Work->Weights,
                  Work->KnotParams,
                  Work->Knots,
                  Work->SampleCount,
                  Work->Ts,
                  Work->OutSamples);
}

internal void
CalcNURBS_SSE_Work(void *UserData)
{
 calc_nurbs_work *Work = Cast(calc_nurbs_work *)UserData;
 CalcNURBS_SSE(Work->Controls,
               Work->Weights,
               Work->KnotParams,
               Work->Knots,
               Work->SampleCount,
               Work->Ts,
               Work->OutSamples);
}

internal void
CalcNURBS_AVX2_Work(void *UserData)
{
 calc_nurbs_work *Work = Cast(calc_nurbs_work *)UserData;
 CalcNURBS_AVX2(Work->Controls,
                Work->Weights,
                Work->KnotParams,
                Work->Knots,
                Work->SampleCount,
                Work->Ts,
                Work->OutSamples);
}

internal void
CalcNURBS_MultiThreaded(v2 *Controls,
                        f32 *Weights,
                        b_spline_knot_params KnotParams,
                        f32 *Knots,
                        u32 SampleCount,
                        f32 *Ts,
                        v2 *OutSamples,
                        work_queue_func *CalcNURBS_Work)
{
 temp_arena Temp = TempArena(0);
 work_queue *WorkQueue = GetCtx()->HighPriorityQueue;
 
 // NOTE(hbr): Experimentally found that 100 is the fastest here. It seems "low" because
 // NURBS evaluation algorithm has O(m^2) complexity, so there is quite a bit of work
 // done there.
 work_queue_blocks Blocks = WorkQueueCalculateBlocks(WorkQueue, SampleCount, 100);
 u32 BlockCount = Blocks.BlockCount;
 u32 BlockSize = Blocks.BlockSize;
 u32 SamplesLeft = SampleCount;
 f32 *TsAt = Ts;
 v2 *OutSamplesAt = OutSamples;
 
 calc_nurbs_work *Works = PushArray(Temp.Arena, BlockCount, calc_nurbs_work);
 
 ForEachIndex(BlockIndex, BlockCount)
 {
  u32 BlockSampleCount = Min(SamplesLeft, BlockSize);
  
  calc_nurbs_work *Work = Works + BlockIndex;
  Work->Controls = Controls;
  Work->Weights = Weights;
  Work->KnotParams = KnotParams;
  Work->Knots = Knots;
  Work->SampleCount = BlockSampleCount;
  Work->Ts = TsAt;
  Work->OutSamples = OutSamplesAt;
  
  Platform.WorkQueueAddEntry(WorkQueue, CalcNURBS_Work, Work);
  
  TsAt += BlockSampleCount;
  OutSamplesAt += BlockSampleCount;
  SamplesLeft -= BlockSampleCount;
 }
 
 Assert(SamplesLeft == 0);
 
 Platform.WorkQueueCompleteAllWork(WorkQueue);
 
 EndTemp(Temp);
}

internal void
CalcNURBS(v2 *Controls,
          f32 *Weights,
          b_spline_knot_params KnotParams,
          f32 *Knots,
          u32 SampleCount,
          f32 *Ts,
          v2 *OutSamples)
{
 ProfileFunctionBegin();
 
 switch (DEBUG_Vars->NURBS_EvalMethod)
 {
  case NURBS_Eval_Scalar_UsingBSplineEvaluate: {
   CalcNURBS_ScalarUsingBSplineEvaluate(Controls, Weights, KnotParams, Knots, SampleCount, Ts, OutSamples);
  }break;
  
  case NURBS_Eval_Scalar: {
   CalcNURBS_Scalar(Controls, Weights, KnotParams, Knots, SampleCount, Ts, OutSamples);
  }break;
  
  case NURBS_Eval_ScalarButUsingSSE: {
   CalcNURBS_ScalarButUsingSSE(Controls, Weights, KnotParams, Knots, SampleCount, Ts, OutSamples);
  }break;
  
  case NURBS_Eval_SSE: {
   CalcNURBS_SSE(Controls, Weights, KnotParams, Knots, SampleCount, Ts, OutSamples);
  }break;
  
  case NURBS_Eval_AVX2: {
   CalcNURBS_AVX2(Controls, Weights, KnotParams, Knots, SampleCount, Ts, OutSamples);
  }break;
  
  case NURBS_Eval_SSE_MultiThreaded: {
   CalcNURBS_MultiThreaded(Controls, Weights, KnotParams, Knots, SampleCount, Ts, OutSamples, CalcNURBS_SSE_Work);
  }break;
  
  case NURBS_Eval_AVX2_MultiThreaded: {
   CalcNURBS_MultiThreaded(Controls, Weights, KnotParams, Knots, SampleCount, Ts, OutSamples, CalcNURBS_AVX2_Work);
  }break;
  
  case NURBS_Eval_Adaptive_MultiThreaded: {
   work_queue_func *EvalFunc = 0;
   
   instruction_set_flags Flags = Platform.InstructionSetSupport();
   if (0) {}
   else if (Flags & InstructionSet_AVX2) EvalFunc = CalcNURBS_AVX2_Work;
   else if (Flags & InstructionSet_SSE) EvalFunc = CalcNURBS_SSE_Work;
   else EvalFunc = CalcNURBS_Scalar_Work;
   
   CalcNURBS_MultiThreaded(Controls, Weights, KnotParams, Knots, SampleCount, Ts, OutSamples, EvalFunc);
  }break;
  
  case NURBS_Eval_Count: InvalidPath;
 }
 
 ProfileEnd();
}

internal void
CalcParametricCurveField(arena *Arena,
                         parametric_curve_field *Field,
                         b32 IsVar,
                         u32 VarCount,
                         string *VarNames,
                         f32 *VarValues)
{
 StructZero(&Field->Cached);
 
 string Equation = StringFromStringId(GetCtx()->StrStore, Field->Equation);
 
 if (IsVar)
 {
  if (Field->EquationOrDragFloatMode_Equation)
  {
   parametric_equation_eval_result Eval = ParametricEquationEval(Arena, Equation, VarCount, VarNames, VarValues);
   Field->Cached.EvalValue = Eval.Value;
   Field->Cached.EvalFail = Eval.Fail;
   Field->Cached.EvalErrorMessage = Eval.ErrorMessage;
  }
 }
 else
 {
  parametric_equation_parse_result Parse = ParametricEquationParse(Arena, Equation, VarCount, VarNames, VarValues);
  Field->Cached.EvalExpr = Parse.ParsedExpr;
  Field->Cached.EvalFail = Parse.Fail;
  Field->Cached.EvalErrorMessage = Parse.ErrorMessage;
 }
}

internal f32
ParametricCurveVarValue(parametric_curve_field *Var)
{
 f32 Value = 0;
 if (Var->EquationOrDragFloatMode_Equation)
 {
  Value = Var->Cached.EvalValue;
 }
 else
 {
  Value = Var->DragValue;
 }
 return Value;
}

internal void
CalcParametric_SingleThreaded(parametric_equation_expr *X_Expr,
                              parametric_equation_expr *Y_Expr,
                              u32 SampleCount,
                              f32 *Ts,
                              v2 *OutSamples)
{
 ForEachIndex(SampleIndex, SampleCount)
 {
  f32 T = Ts[SampleIndex];
  
  f32 X = ParametricEquationEvalWithT(X_Expr, T);
  f32 Y = ParametricEquationEvalWithT(Y_Expr, T);
  
  OutSamples[SampleIndex] = V2(X, Y);
 }
}

struct calc_parametric_work
{
 parametric_equation_expr *X_Expr;
 parametric_equation_expr *Y_Expr;
 u32 SampleCount;
 f32 *Ts;
 v2 *OutSamples;
};

internal void
CalcParametric_Work(void *UserData)
{
 calc_parametric_work *Work = Cast(calc_parametric_work *)UserData;
 CalcParametric_SingleThreaded(Work->X_Expr, Work->Y_Expr, Work->SampleCount, Work->Ts, Work->OutSamples);
}

internal void
CalcParametric_MultiThreaded(parametric_equation_expr *X_Expr,
                             parametric_equation_expr *Y_Expr,
                             u32 SampleCount,
                             f32 *Ts,
                             v2 *OutSamples)
{
 ProfileFunctionBegin();
 
 temp_arena Temp = TempArena(0);
 work_queue *WorkQueue = GetCtx()->HighPriorityQueue;
 
 // NOTE(hbr): 256 selected experimentally
 work_queue_blocks Blocks = WorkQueueCalculateBlocks(WorkQueue, SampleCount, 256);
 u32 BlockCount = Blocks.BlockCount;
 u32 BlockSize = Blocks.BlockSize;
 u32 SamplesLeft = SampleCount;
 f32 *TsAt = Ts;
 v2 *OutSamplesAt = OutSamples;
 
 calc_parametric_work *Works = PushArray(Temp.Arena, BlockCount, calc_parametric_work);
 
 ForEachIndex(BlockIndex, BlockCount)
 {
  u32 BlockSampleCount = Min(SamplesLeft, BlockSize);
  
  calc_parametric_work *Work = Works + BlockIndex;
  Work->X_Expr = X_Expr;
  Work->Y_Expr = Y_Expr;
  Work->SampleCount = BlockSampleCount;
  Work->Ts = TsAt;
  Work->OutSamples = OutSamplesAt;
  
  Platform.WorkQueueAddEntry(WorkQueue, CalcParametric_Work, Work);
  
  TsAt += BlockSampleCount;
  OutSamplesAt += BlockSampleCount;
  SamplesLeft -= BlockSampleCount;
 }
 
 Platform.WorkQueueCompleteAllWork(WorkQueue);
 
 EndTemp(Temp);
 
 ProfileEnd();
}

internal void
CalcParametric(arena *Arena,
               parametric_curve_resources *Parametric,
               u32 SampleCount, v2 *OutSamples)
{
 ProfileFunctionBegin();
 
 temp_arena Temp = TempArena(Arena);
 
 u32 MaxVarCount = ArrayCount(Parametric->AdditionalVars);
 u32 VarCount = 0;
 string *VarNames = PushArray(Temp.Arena, MaxVarCount, string);
 f32 *VarValues = PushArray(Temp.Arena, MaxVarCount, f32);
 
 ForEachElement(AdditionalVarIndex, Parametric->AdditionalVars)
 {
  parametric_curve_field *Var = Parametric->AdditionalVars + AdditionalVarIndex;
  if (IsParametricCurveVarActive(Var))
  {
   string VarName = StringFromStringId(Var->VarName);
   CalcParametricCurveField(Arena, Var, true, VarCount, VarNames, VarValues);
   f32 VarValue = ParametricCurveVarValue(Var);
   
   Assert(VarCount < MaxVarCount);
   VarNames[VarCount] = VarName;
   VarValues[VarCount] = VarValue;
   ++VarCount;
  }
 }
 
 CalcParametricCurveField(Arena, &Parametric->X_Equation, false, VarCount, VarNames, VarValues);
 CalcParametricCurveField(Arena, &Parametric->Y_Equation, false, VarCount, VarNames, VarValues);
 
 parametric_equation_expr *X_Expr = Parametric->X_Equation.Cached.EvalExpr;
 parametric_equation_expr *Y_Expr = Parametric->Y_Equation.Cached.EvalExpr;
 
 CalcParametricCurveField(Arena, &Parametric->MinT_Var, true, VarCount, VarNames, VarValues);
 CalcParametricCurveField(Arena, &Parametric->MaxT_Var, true, VarCount, VarNames, VarValues);
 
 f32 MinT = ParametricCurveVarValue(&Parametric->MinT_Var);
 f32 MaxT = ParametricCurveVarValue(&Parametric->MaxT_Var);
 // NOTE(hbr): Adjust for safety
 MaxT = Max(MinT, MaxT);
 
 f32 T = MinT;
 f32 DeltaT = (MaxT - MinT) / (SampleCount - 1);
 f32 *Ts = PushArrayNonZero(Temp.Arena, SampleCount, f32);
 ForEachIndex(SampleIndex, SampleCount)
 {
  Ts[SampleIndex] = T;
  T += DeltaT;
 }
 
 switch (DEBUG_Vars->Parametric_EvalMethod)
 {
  case Parametric_Eval_SingleThreaded: {
   CalcParametric_SingleThreaded(X_Expr, Y_Expr, SampleCount, Ts, OutSamples);
  }break;
  
  case Parametric_Eval_MultiThreaded: {
   CalcParametric_MultiThreaded(X_Expr, Y_Expr, SampleCount, Ts, OutSamples);
  }break;
  
  case Parametric_Eval_Count: InvalidPath;
 }
 
 EndTemp(Temp);
 
 ProfileEnd();
}

internal void
CalcCurve(curve *Curve, u32 SampleCount, v2 *OutSamples)
{
 temp_arena Temp = TempArena(0);
 
 curve_points_static *Points = GetCurvePoints(Curve);
 u32 PointCount = Points->ControlPointCount;
 v2 *Controls = Points->ControlPoints;
 f32 *Weights = Points->ControlPointWeights;
 cubic_bezier_point *Beziers = Points->CubicBezierPoints;
 curve_params *Params = &Curve->Params;
 f32 *BSplineKnots = Points->BSplineKnots;
 u32 SamplesPerControlPoint = Params->SamplesPerControlPoint;
 parametric_curve_resources *Parametric = &Curve->ParametricResources;
 arena *ComputeArena = Curve->ComputeArena;
 
 switch (Params->Type)
 {
  case Curve_Polynomial: {CalcPolynomial(Controls, PointCount, Params->Polynomial, SampleCount, OutSamples, SamplesPerControlPoint);} break;
  case Curve_CubicSpline: {CalcCubicSpline(Controls, PointCount, Params->CubicSpline, SampleCount, OutSamples);} break;
  
  case Curve_Bezier: {
   switch (Params->Bezier)
   {
    case Bezier_Rational: {CalcBezierRational(Controls, Weights, PointCount, SampleCount, OutSamples);}break;
    case Bezier_CubicSpline: {CalcBezierCubicSpline(Beziers, PointCount, SampleCount, OutSamples);}break;
    case Bezier_Count: InvalidPath;
   }
  } break;
  
  case Curve_NURBS: {
   MaybeRecomputeCurveBSplineKnots(Curve, false);
   
   b_spline_knot_params KnotParams = GetBSplineParams(Curve).KnotParams;
   u32 PartitionSize = KnotParams.PartitionSize;
   u32 Degree = KnotParams.Degree;
   Points->BSplineKnotCount = KnotParams.KnotCount;
   
   v2 *PartitionKnots = PushArrayNonZero(Curve->ComputeArena, PartitionSize, v2);
   CalcNURBS(Controls,
             Weights,
             KnotParams,
             BSplineKnots,
             PartitionSize,
             BSplineKnots + Degree,
             PartitionKnots);
   
   Curve->BSplinePartitionKnots = PartitionKnots;
   
   f32 T = KnotParams.A;
   f32 Delta_T = (KnotParams.B - KnotParams.A) / (SampleCount - 1);
   f32 *Ts = PushArrayNonZero(Temp.Arena, SampleCount, f32);
   ForEachIndex(SampleIndex, SampleCount)
   {
    Ts[SampleIndex] = T;
    T += Delta_T;
   }
   CalcNURBS(Controls,
             Weights,
             KnotParams,
             BSplineKnots,
             SampleCount,
             Ts,
             OutSamples);
  }break;
  
  case Curve_Parametric: {CalcParametric(ComputeArena, Parametric, SampleCount, OutSamples);}break;
  
  case Curve_Count: InvalidPath;
 }
 
 EndTemp(Temp);
}

// TODO(hbr): This function needs some serious refactoring.
// First of all, don't break down all the "Compute" functions into hierarchical structure.
// It's very fragile. It's just better (I think) to duplicate some code but have it more independent.
// Second of all just refactor stupid code duplication here.
// Basically inline everything and work your way up from there.
// Then parallelize this heavily. I mean, we can unroll, we can simd, we can multithread this.
internal void
RecomputeCurve(curve *Curve)
{
 ProfileFunctionBegin();
 
 curve_params *Params = &Curve->Params;
 arena *ComputeArena = Curve->ComputeArena;
 curve_points_static *Points = GetCurvePoints(Curve);
 u32 ControlCount = Points->ControlPointCount;
 v2 *Controls = Points->ControlPoints;
 f32 *Weights = Points->ControlPointWeights;
 f32 LineWidth = Params->DrawParams.Line.Width;
 
 ClearArena(ComputeArena);
 
 u32 SampleCount = 0;
 if (IsCurveTotalSamplesMode(Curve))
 {
  SampleCount = Curve->Params.TotalSamples;
 }
 else
 {
  u32 ControlCountWithLooped = (IsCurveLooped(Curve) ? ControlCount + 1 : ControlCount);
  if (ControlCountWithLooped > 0)
  {
   SampleCount = (ControlCountWithLooped - 1) * Params->SamplesPerControlPoint + 1;
  }
 }
 v2 *Samples = PushArrayNonZero(ComputeArena, SampleCount, v2);
 CalcCurve(Curve, SampleCount, Samples);
 
 vertex_array CurveVertices = ComputeVerticesOfThickLine(ComputeArena, SampleCount, Samples, LineWidth, IsCurveLooped(Curve));
 vertex_array PolylineVertices = ComputeVerticesOfThickLine(ComputeArena, ControlCount, Controls, Params->DrawParams.Polyline.Width, false);
 
 v2 *ConvexHullPoints = PushArrayNonZero(ComputeArena, ControlCount, v2);
 u32 ConvexHullCount = CalcConvexHull(ControlCount, Controls, ConvexHullPoints);
 vertex_array ConvexHullVertices = ComputeVerticesOfThickLine(ComputeArena, ConvexHullCount, ConvexHullPoints, Params->DrawParams.ConvexHull.Width, true);
 
 point_tracking_along_curve_state *Tracking = &Curve->PointTracking;
 if (IsCurveEligibleForPointTracking(Curve))
 {
  if (Tracking->Type != PointTrackingAlongCurve_None)
  {
   f32 Fraction = Tracking->Fraction;
   
   v2 LocalSpaceTrackedPoint = BezierCurveRationalEvaluateScalar(Fraction, Controls, Weights, ControlCount);
   Tracking->LocalSpaceTrackedPoint = LocalSpaceTrackedPoint;
   
   if (Tracking->Type == PointTrackingAlongCurve_DeCasteljauVisualization)
   {
    all_de_casteljau_intermediate_results Intermediate = DeCasteljauAlgorithm(ComputeArena, Fraction, Controls, Weights, ControlCount);
    vertex_array *LineVerticesPerIteration = PushArray(ComputeArena, Intermediate.IterationCount, vertex_array);
    
    u32 IterationPointsOffset = 0;
    for (u32 Iteration = 0;
         Iteration < Intermediate.IterationCount;
         ++Iteration)
    {
     u32 CurrentIterationPointCount = Intermediate.IterationCount - Iteration;
     LineVerticesPerIteration[Iteration] = ComputeVerticesOfThickLine(ComputeArena,
                                                                      CurrentIterationPointCount,
                                                                      Intermediate.P + IterationPointsOffset,
                                                                      LineWidth,
                                                                      false);
     IterationPointsOffset += CurrentIterationPointCount;
    }
    
    Tracking->Intermediate = Intermediate;
    Tracking->LineVerticesPerIteration = LineVerticesPerIteration;
    Tracking->LocalSpaceTrackedPoint = Intermediate.P[Intermediate.TotalPointCount - 1];
   }
  }
 }
 else
 {
  Tracking->Type = PointTrackingAlongCurve_None;
 }
 
 u32 BSplineConvexHullCount = 0;
 b_spline_convex_hull *BSplineConvexHulls = 0;
 if (IsBSplineCurve(Curve))
 {
  b_spline_knot_params BSpline = GetBSplineParams(Curve).KnotParams;
  u32 Degree = BSpline.Degree;
  Assert(Degree <= ControlCount);
  u32 ConvexHullCount = ControlCount - Degree;
  b_spline_convex_hull *Hulls = PushArray(ComputeArena, ConvexHullCount, b_spline_convex_hull);
  ForEachIndex(ConvexHullIndex, ConvexHullCount)
  {
   u32 PointCount = Degree + 1;
   v2 *Points = PushArray(ComputeArena, PointCount, v2);
   PointCount = CalcConvexHull(PointCount, Controls + ConvexHullIndex, Points);
   vertex_array Vertices = ComputeVerticesOfThickLine(ComputeArena, PointCount, Points, Params->DrawParams.BSplinePartialConvexHull.Width, true);
   b_spline_convex_hull *Hull = Hulls + ConvexHullIndex;
   Hull->Points = Points;
   Hull->Vertices = Vertices;
  }
  BSplineConvexHullCount = ConvexHullCount;
  BSplineConvexHulls = Hulls;
 }
 
 Curve->CurveSampleCount = SampleCount;
 Curve->CurveSamples = Samples;
 Curve->CurveVertices = CurveVertices;
 Curve->PolylineVertices = PolylineVertices;
 Curve->ConvexHullPoints = ConvexHullPoints;
 Curve->ConvexHullCount = ConvexHullCount;
 Curve->ConvexHullVertices = ConvexHullVertices;
 Curve->BSplineConvexHullCount = BSplineConvexHullCount;
 Curve->BSplineConvexHulls = BSplineConvexHulls;
 
 ProfileEnd();
}

internal void
MarkEntityModified(entity_with_modify_witness *Witness)
{
 Witness->Modified = true;
}

internal entity_with_modify_witness
BeginEntityModify(entity *Entity)
{
 entity_with_modify_witness Result = {};
 Result.Entity = Entity;
 return Result;
}

internal void
EndEntityModify(entity_with_modify_witness Witness)
{
 entity *Entity = Witness.Entity;
 if (Entity && Witness.Modified)
 {
  switch (Entity->Type)
  {
   case Entity_Curve: {RecomputeCurve(&Entity->Curve);}break;
   case Entity_Image: {}break;
   case Entity_Count: InvalidPath;
  }
  
  ++Entity->Version;
 }
}

internal curve_params
DefaultCurveParams(void)
{
 curve_params Params = {};
 
 f32 LineWidth = 0.009f;
 rgba PolylineColor = RGBA_U8(16, 31, 31, 200);
 
 Params.DrawParams.Line.Enabled = true;
 Params.DrawParams.Line.Color = RGBA_U8(21, 69, 98);
 Params.DrawParams.Line.Width = LineWidth;
 Params.DrawParams.Points.Enabled = true;
 Params.DrawParams.Points.Color = RGBA_U8(0, 138, 138, 148);
 Params.DrawParams.Points.Radius = 0.014f;
 Params.DrawParams.Polyline.Color = PolylineColor;
 Params.DrawParams.Polyline.Width = LineWidth;
 Params.DrawParams.ConvexHull.Color = PolylineColor;
 Params.DrawParams.ConvexHull.Width = LineWidth;
 Params.DrawParams.BSplineKnots.Radius = 0.010f;
 Params.DrawParams.BSplineKnots.Color = RGBA_U8(138, 0, 0, 148);
 Params.DrawParams.BSplinePartialConvexHull.Color = PolylineColor;
 Params.DrawParams.BSplinePartialConvexHull.Width = LineWidth;
 
 Params.SamplesPerControlPoint = 50;
 Params.TotalSamples = 1000;
 
 return Params;
}

internal string_store *
AllocStringStore(arena_store *ArenaStore)
{
 arena *Arena = AllocArenaFromStore(ArenaStore, Gigabytes(1));
 string_store *Store = PushStruct(Arena, string_store);
 Store->StrCache = AllocStringCache(Arena);
 Store->Arena = Arena;
 Store->ArenaStore = ArenaStore;
 return Store;
}

internal string_id
AllocStringFromString(string_store *Store, string Str)
{
 string_id Id = StringIdFromIndex(Store->StrCount);
 SetOrAllocStringOfId(Store, Str, Id);
 return Id;
}

internal void
DeallocStringFromStore(string_store *Store, string_id Id)
{
 char_buffer *Buffer = CharBufferFromStringId(Store, Id);
 DeallocString(Store->StrCache, *Buffer);
 StructZero(Buffer);
}

internal void
SetOrAllocStringOfId(string_store *Store, string Str, string_id Id)
{
 u32 Index = IndexFromStringId(Id);
 if (Index >= Store->StrCount)
 {
  string_id AllocatedId = AllocStringOfSize(Store, Str.Count);
  Assert(StringIdMatch(Id, AllocatedId));
 }
 char_buffer *Buffer = CharBufferFromStringId(Store, Id);
 FillCharBuffer(Buffer, Str);
}

internal string_id
AllocStringOfSize(string_store *Store, u64 Size)
{
 char_buffer Buffer = AllocString(Store->StrCache, Size);
 if (Store->StrCount >= Store->StrCapacity)
 {
  u32 NewCapacity = Max(2 * Store->StrCapacity, Store->StrCount + 1);
  char_buffer *NewStrs = PushArrayNonZero(Store->Arena, NewCapacity, char_buffer);
  ArrayCopy(NewStrs, Store->Strs, Store->StrCount);
  Store->Strs = NewStrs;
  Store->StrCapacity = NewCapacity;
 }
 Assert(Store->StrCount < Store->StrCapacity);
 Store->Strs[Store->StrCount] = Buffer;
 u32 Index = Store->StrCount++;
 string_id Id = {Index};
 return Id;
}

internal char_buffer *
CharBufferFromStringId(string_store *Store, string_id Id)
{
 Assert(Id.Index < Store->StrCount);
 char_buffer *Buffer = Store->Strs + Id.Index;
 return Buffer;
}

internal string
StringFromStringId(string_store *Store, string_id Id)
{
 char_buffer *Buffer = CharBufferFromStringId(Store, Id);
 string Str = StrFromCharBuffer(*Buffer);
 return Str;
}

internal string_id
StringIdFromIndex(u32 Index)
{
 string_id Id = {};
 Id.Index = Index;
 return Id;
}

internal u32
IndexFromStringId(string_id Id)
{
 u32 Index = Id.Index;
 return Index;
}

internal b32
StringIdMatch(string_id A, string_id B)
{
 b32 Result = (A.Index == B.Index);
 return Result;
}

internal change_project_method
CopyProjectChangeMethod(arena *Arena, change_project_method Method)
{
 change_project_method Result = {};
 Result.Type = Method.Type;
 Result.FilePath = StrCopy(Arena, Method.FilePath);
 return Result;
}

internal parametric_curve_field *
AllocParametricCurveVar(parametric_curve_resources *Resources)
{
 parametric_curve_field *Result = 0;
 parametric_curve_field *Free = Resources->AdditionalVars;
 for (u32 Index = 0;
      Index < ArrayCount(Resources->AdditionalVars);
      ++Index)
 {
  parametric_curve_field *Var = Resources->AdditionalVars + Index;
  if (IsParametricCurveVarActive(Var))
  {
   Swap(*Free, *Var, parametric_curve_field);
   ++Free;
  }
 }
 ForEachElement(Index, Resources->AdditionalVars)
 {
  parametric_curve_field *Var = Resources->AdditionalVars + Index;
  if (!IsParametricCurveVarActive(Var))
  {
   string_store *StrStore = GetCtx()->StrStore;
   Var->Id = ++Resources->AdditionalVarIndexCounter;
   Var->VarName = AllocStringOfSize(StrStore, 10);
   Var->Equation = AllocStringOfSize(StrStore, 64);
   ++Resources->AdditionalVarCount;
   Result = Var;
   break;
  }
 }
 return Result;
}

internal void
DeallocParametricCurveVar(parametric_curve_resources *Resources, parametric_curve_field *Var)
{
 string_store *StrStore = GetCtx()->StrStore;
 DeallocStringFromStore(StrStore, Var->VarName);
 DeallocStringFromStore(StrStore, Var->Equation);
 StructZero(Var);
 Assert(Resources->AdditionalVarCount > 0);
 --Resources->AdditionalVarCount;
}

internal b32
ParametricCurveResourcesHasFreeAddditionalVar(parametric_curve_resources *Resources)
{
 b32 Result = (Resources->AdditionalVarCount < ArrayCount(Resources->AdditionalVars));
 return Result;
}

internal b32
IsParametricCurveVarActive(parametric_curve_field *Var)
{
 b32 Result = (Var->Id != 0);
 return Result;
}

internal void
LoadParametricCurvePredefinedExample(curve *Curve, parametric_curve_predefined_example_type PredefinedExample)
{
 parametric_curve_resources *Resources = &Curve->ParametricResources;
 parametric_curve_predefined_example Example = ParametricCurvePredefinedExamples[PredefinedExample];
 
 FillCharBuffer(Resources->X_Equation.Equation, Example.X_Equation);
 FillCharBuffer(Resources->Y_Equation.Equation, Example.Y_Equation);
 
 FillCharBuffer(Resources->MinT_Var.VarName, Example.Min_T.Name);
 FillCharBuffer(Resources->MaxT_Var.VarName, Example.Max_T.Name);
 
 FillCharBuffer(Resources->MinT_Var.Equation, Example.Min_T.Equation);
 FillCharBuffer(Resources->MaxT_Var.Equation, Example.Max_T.Equation);
 
 Resources->MinT_Var.EquationOrDragFloatMode_Equation = Example.Min_T.EquationMode;
 Resources->MinT_Var.DragValue = Example.Min_T.Value;
 
 Resources->MaxT_Var.EquationOrDragFloatMode_Equation = Example.Max_T.EquationMode;
 Resources->MaxT_Var.DragValue = Example.Max_T.Value;
 
 // TODO(hbr): First deallocate everything
 ForEachElement(AdditionalVarIndex, Resources->AdditionalVars)
 {
  parametric_curve_field *Var = Resources->AdditionalVars + AdditionalVarIndex;
  if (IsParametricCurveVarActive(Var))
  {
   DeallocParametricCurveVar(Resources, Var);
  }
 }
 
 ForEachElement(AdditionalVar, Example.AdditionalVars)
 {
  parametric_curve_predefined_example_var *SrcVar = Example.AdditionalVars + AdditionalVar;
  if (SrcVar->Name.Count > 0)
  {
   parametric_curve_field *DstVar = AllocParametricCurveVar(Resources);
   Assert(DstVar);
   
   FillCharBuffer(DstVar->VarName, SrcVar->Name);
   FillCharBuffer(DstVar->Equation, SrcVar->Equation);
   
   DstVar->EquationOrDragFloatMode_Equation = SrcVar->EquationMode;
   DstVar->DragValue = SrcVar->Value;
  }
 }
}