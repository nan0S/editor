#ifndef EDITOR_PROJECT_H
#define EDITOR_PROJECT_H

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
   name_string CurveName;
   
   curve_params CurveParams;
   
   world_position Position;
   rotation_2d Rotation;
   
   u64 NumControlPoints;
   
   b32 IsSelected;
   u64 SelectedControlPointIndex;
};

struct saved_project_image
{
   name_string ImageName;
   
   u64 ImagePathSize;
   
   v2f32 Position;
   v2f32 Scale;
   rotation_2d Rotation;
   
   s64 SortingLayer;
   
   b32 IsSelected;
   b32 Hidden;
};

struct saved_project_entity
{
   entity_type Type;
   saved_project_curve Curve;
   saved_project_image Image;
};

struct load_project_result
{
   editor_state EditorState;
   editor_parameters EditorParameters;
   ui_config UI_Config;
   
   error_string Error;
   string_list Warnings;
};

function saved_project_header SavedProjectHeader(u64 MagicValue, u64 NumEntities,
                                                 u64 CurveCounter, camera Camera,
                                                 editor_parameters EditorParameters,
                                                 ui_config UI_Config,
                                                 f32 CurveAnimationSpeed);
function saved_project_curve SavedProjectCurve(name_string CurveName, curve_params CurveParams,
                                               world_position Position, rotation_2d Rotation,
                                               u64 NumControlPoints, b32 IsSelected,
                                               u64 SelectedControlPointIndex);
function saved_project_image SavedProjectImage(name_string ImageName, u64 ImagePathSize,
                                               v2f32 Position, v2f32 Scale, rotation_2d Rotation,
                                               u64 SortingLayer, b32 IsSelected, b32 Hidden);

function error_string SaveProjectInFile(arena *Arena,
                                        editor Editor,
                                        string SaveFilePath);
function load_project_result LoadProjectFromFile(arena *Arena,
                                                 pool *EntityPool,
                                                 arena *DeCasteljauVisualizationArena,
                                                 arena *DegreeLoweringArena,
                                                 arena *MovingPointArena,
                                                 arena *CurveAnimationArena,
                                                 string ProjectFilePath);

#endif //EDITOR_PROJECT_H
