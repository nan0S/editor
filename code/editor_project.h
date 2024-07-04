#ifndef EDITOR_PROJECT_H
#define EDITOR_PROJECT_H

internal error_string SaveProjectInFile(arena *Arena, editor Editor, string SaveFilePath);
internal load_project_result LoadProjectFromFile(arena *Arena,
                                                 string ProjectFilePath,
                                                 pool *EntityPool,
                                                 arena *DeCasteljauVisualizationArena,
                                                 arena *DegreeLoweringArena,
                                                 arena *MovingPointArena,
                                                 arena *CurveAnimationArena);

#endif //EDITOR_PROJECT_H
