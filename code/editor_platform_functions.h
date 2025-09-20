/* ========================================================================
   Parametric Curve Editor
   Master's Thesis by Hubert Obrzut
   Supervisor: Paweł Woźny
   University of Wrocław
   Faculty of Mathematics and Computer Science
   Institute of Computer Science
   Date: September 2025
   ======================================================================== */

#ifndef EDITOR_PLATFORM_FUNCTIONS_H
#define EDITOR_PLATFORM_FUNCTIONS_H

#define EDITOR_UPDATE_AND_RENDER(Name) void Name(struct editor_memory *Memory, struct platform_input_output *Input, struct render_frame *Frame)
typedef EDITOR_UPDATE_AND_RENDER(editor_update_and_render);

#define EDITOR_ON_CODE_RELOAD(Name) void Name(struct editor_memory *Memory)
typedef EDITOR_ON_CODE_RELOAD(editor_on_code_reload);

#endif //EDITOR_PLATFORM_FUNCTIONS_H
