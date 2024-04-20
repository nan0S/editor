#ifndef EDITOR_PROJECT_H
#define EDITOR_PROJECT_H

// TODO(hbr): Remove this type as well, just save [editor].
struct load_project_result
{
   editor_state EditorState;
   editor_parameters EditorParameters;
   ui_config UI_Config;
   
   error_string Error;
   string_list Warnings;
};

function error_string SaveProjectInFile(arena *Arena, editor Editor, string SaveFilePath);
function load_project_result LoadProjectFromFile(arena *Arena,
                                                 pool *EntityPool,
                                                 arena *DeCasteljauVisualizationArena,
                                                 arena *DegreeLoweringArena,
                                                 arena *MovingPointArena,
                                                 arena *CurveAnimationArena,
                                                 string ProjectFilePath);

#endif //EDITOR_PROJECT_H
