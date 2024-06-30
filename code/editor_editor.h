#ifndef EDITOR_EDITOR_H
#define EDITOR_EDITOR_H

#define WINDOW_TITLE "Parametric Curves Editor"

#define SAVED_PROJECT_FILE_EXTENSION ".editor"

// NOTE(hbr): This is a little too complex for such a simple thing.
#define PROJECT_FILE_EXTENSION_SELECTION "Project File (*" SAVED_PROJECT_FILE_EXTENSION ")"
#define JPG_FILE_EXTENSION_SELECTION "JPG Image (*.jpg)"
#define PNG_FILE_EXTENSION_SELECTION "PNG Image (*.png)"

#define SAVE_AS_MODAL_EXTENSION_SELECTION \
PROJECT_FILE_EXTENSION_SELECTION "{" SAVED_PROJECT_FILE_EXTENSION "}" \
"," JPG_FILE_EXTENSION_SELECTION "{.jpg}" \
"," PNG_FILE_EXTENSION_SELECTION "{.png}"

// NOTE(hbr): Scale in respect to window size
global f32 NotificationWindowPaddingScale = 0.01f;
// TODO(hbr): Do something with this variable. Don't want to load project on different monitor
// and images to be different size.
global f32 GlobalImageScaleFactor         = 1.0f / 1920.0f;

#define ROTATION_INDICATOR_DEFAULT_RADIUS_CLIP_SPACE          0.06f
#define ROTATION_INDICATOR_DEFAULT_OUTLINE_THICKNESS_FRACTION 0.1f
#define ROTATION_INDICATOR_DEFAULT_FILL_COLOR                 ColorMake(30, 56, 87, 80)
#define ROTATION_INDICATOR_DEFAULT_OUTLINE_COLOR              ColorMake(255, 255, 255, 24)

#define BEZIER_SPLIT_POINT_DEFAULT_RADIUS_CLIP_SPACE          0.025f
#define BEZIER_SPLIT_POINT_DEFAULT_OUTLINE_THICKNESS_FRACTION 0.1f
#define BEZIER_SPLIT_POINT_DEFAULT_FILL_COLOR                 ColorMake(0, 255, 0, 100)
#define BEZIER_SPLIT_POINT_DEFAULT_OUTLINE_COLOR              ColorMake(0, 200, 0, 200)

#define DEFAULT_BACKGROUND_COLOR                                     ColorMake(21, 21, 21)
#define DEFAULT_COLLLISION_TOLERANCE_CLIP_SPACE                      0.02f
#define LAST_CONTROL_POINT_DEFAULT_SIZE_MULTIPLIER                   1.5f
#define SELECTED_CURVE_CONTROL_POINT_DEFAULT_OUTLINE_THICKNESS_SCALE 0.55f
#define SELECTED_CURVE_CONTROL_POINT_DEFAULT_OUTLINE_COLOR           ColorMake(1, 52, 49, 209)
#define SELECTED_CONTROL_POINT_DEFAULT_OUTLINE_COLOR                 ColorMake(58, 183, 183, 177)
#define CUBIC_BEZIER_HELPER_LINE_DEFAULT_WIDTH_CLIP_SPACE            0.003f

#define CURVE_DEFAULT_CURVE_SHAPE_PARAMS                    CurveShapeMake(Interpolation_CubicSpline, \
PolynomialInterpolation_Barycentric, \
PointsArrangement_Chebychev, \
CubicSpline_Natural, \
Bezier_Normal)
#define CURVE_DEFAULT_CURVE_WIDTH                           0.009f
#define CURVE_DEFAULT_CURVE_COLOR                           ColorMake(21, 69, 98)
#define CURVE_DEFAULT_POINTS_DISABLED                       false
#define CURVE_DEFAULT_POINT_SIZE                            0.014f
#define CURVE_DEFAULT_POINT_COLOR                           ColorMake(0, 138, 138, 148)
#define CURVE_DEFAULT_POLYLINE_ENABLED                      false
#define CURVE_DEFAULT_POLYLINE_WIDTH                        CURVE_DEFAULT_CURVE_WIDTH
#define CURVE_DEFAULT_POLYLINE_COLOR                        ColorMake(16, 31, 31, 200)
#define CURVE_DEFAULT_CONVEX_HULL_ENABLED                   false         
#define CURVE_DEFAULT_CONVEX_HULL_WIDTH                     CURVE_DEFAULT_CURVE_WIDTH
#define CURVE_DEFAULT_CONVEX_HULL_COLOR                     CURVE_DEFAULT_POLYLINE_COLOR
#define CURVE_DEFAULT_NUM_CURVE_POINTS_PER_SEGMENT          50
#define CURVE_DEFAULT_DE_CASTELJEU_VISUALIZATION_GRADIENT_A ColorMake(255, 0, 144)
#define CURVE_DEFAULT_DE_CASTELJEU_VISUALIZATION_GRADIENT_B ColorMake(155, 200, 0)
#define CURVE_DEFAULT_CONTROL_POINT_WEIGHT                  1.0f
#define CURVE_DEFAULT_CURVE_PARAMS \
CurveParamsMake(CURVE_DEFAULT_CURVE_SHAPE_PARAMS, \
CURVE_DEFAULT_CURVE_WIDTH, \
CURVE_DEFAULT_CURVE_COLOR, \
CURVE_DEFAULT_POINTS_DISABLED, \
CURVE_DEFAULT_POINT_SIZE, \
CURVE_DEFAULT_POINT_COLOR, \
CURVE_DEFAULT_POLYLINE_ENABLED, \
CURVE_DEFAULT_POLYLINE_WIDTH, \
CURVE_DEFAULT_POLYLINE_COLOR, \
CURVE_DEFAULT_CONVEX_HULL_ENABLED, \
CURVE_DEFAULT_CONVEX_HULL_WIDTH, \
CURVE_DEFAULT_CONVEX_HULL_COLOR, \
CURVE_DEFAULT_NUM_CURVE_POINTS_PER_SEGMENT, \
CURVE_DEFAULT_DE_CASTELJEU_VISUALIZATION_GRADIENT_A, \
CURVE_DEFAULT_DE_CASTELJEU_VISUALIZATION_GRADIENT_B)
#define CURVE_DEFAULT_ANIMATION_SPEED                       1.0f

struct render_point_data
{
   f32 RadiusClipSpace;
   f32 OutlineThicknessFraction;
   color FillColor;
   color OutlineColor;
};

struct editor_parameters
{
   render_point_data RotationIndicator;
   render_point_data BezierSplitPoint;
   
   color BackgroundColor;
   f32   CollisionToleranceClipSpace;
   f32   LastControlPointSizeMultiplier;
   f32   SelectedCurveControlPointOutlineThicknessScale;
   color SelectedCurveControlPointOutlineColor;
   color SelectedControlPointOutlineColor;
   f32   CubicBezierHelperLineWidthClipSpace;
   
   curve_params CurveDefaultParams;
   f32 DefaultControlPointWeight;
};

struct camera
{
   world_position Position;
   rotation_2d Rotation;
   
   f32 Zoom;
   f32 ZoomSensitivity;
   
   b32 ReachingTarget;
   f32 ReachingTargetSpeed;
   world_position PositionTarget;
   f32 ZoomTarget;
};

struct projection
{
   f32 AspectRatio;
   f32 FrustumSize;
};

struct coordinate_system_data
{
   sf::RenderWindow *Window;
   camera Camera;
   projection Projection;
};

internal camera_position ScreenToCameraSpace(screen_position Position, coordinate_system_data Data);
internal world_position  CameraToWorldSpace (camera_position Position, coordinate_system_data Data);
internal camera_position WorldToCameraSpace (world_position Position,  coordinate_system_data Data);
internal world_position  ScreenToWorldSpace (screen_position Position, coordinate_system_data Data);

// NOTE(hbr): Distance in space [-AspectRatio, AspectRatio] x [-1, 1]
internal f32 ClipSpaceLengthToWorldSpace(f32 ClipSpaceDistance, coordinate_system_data Data);

enum curve_collision_type
{
   CurveCollision_ControlPoint,
   CurveCollision_CurvePoint,
   CurveCollision_CubicBezierPoint,
};

struct collision
{
   // NOTE(hbr): Fat-struct to reduce complexity
   entity *Entity;
   curve_collision_type Type;
   u64 PointIndex;
};

typedef u64 check_collision_with_flags;
enum
{
   CheckCollisionWith_ControlPoints     = (1<<0),
   CheckCollisionWith_CurvePoints       = (1<<1),
   CheckCollisionWith_Images            = (1<<2),
};

enum user_action_type
{
   UserAction_None,
   UserAction_ButtonClicked,
   UserAction_ButtonDrag,
   UserAction_ButtonReleased,
   UserAction_MouseMove,
};

struct user_action
{
   user_action_type Type;
   union
   {
      struct {
         button Button;
         screen_position ClickPosition;
      } Click;
      
      struct {
         button Button;
         screen_position DragFromPosition;
      } Drag;
      
      struct {
         button Button;
         screen_position ReleasePosition;
      } Release;
      
      struct {
         screen_position FromPosition;
         screen_position ToPosition;
      } MouseMove;
   };
   
   user_input *UserInput;
};

internal user_action UserActionButtonClicked(button Button, screen_position ClickPosition, user_input *UserInput);
internal user_action UserActionButtonDrag(button Button, screen_position DragFromPosition, user_input *UserInput);
internal user_action UserActionButtonReleased(button Button, screen_position ReleasePosition, user_input *UserInput);
internal user_action UserActionMouseMove(v2s32 FromPosition, screen_position ToPosition, user_input *UserInput);

enum editor_mode_type
{
   EditorMode_Normal,
   EditorMode_Moving,
   EditorMode_Rotating,
};

// TODO(hbr): Try to get rid of this type, if this is necessary, just remove this TODO
enum moving_mode_type
{
   MovingMode_Entity,
   MovingMode_Camera,
   MovingMode_CurvePoint,
};

struct editor_mode
{
   editor_mode_type Type;
   union
   {
      struct {
         moving_mode_type Type;
         entity *Entity;
         
         u64 PointIndex;
         b32 CubicBezierPointMoving;
         
         sf::Vertex *SavedCurveVertices;
         u64 SavedNumCurveVertices;
         sf::PrimitiveType SavedPrimitiveType;
      } Moving;
      
      struct {
         entity *Entity;
         screen_position RotationCenter;
      } Rotating;
   };
};

// TODO(hbr): maybe rename to [state] or something, not only this but [de_casteljau_visualization] as well, because that's what it is
struct splitting_bezier_curve
{
   // TODO(hbr): Get rid of [IsSplitting], encode in [SplitCurveEntity]
   b32 IsSplitting;
   entity *SplitCurveEntity;
   f32 T;
   b32 DraggingSplitPoint;
   u64 SavedCurveVersion;
   world_position SplitPoint;
};

struct de_casteljau_visualization
{
   b32 IsVisualizing;
   f32 T;
   
   entity *CurveEntity;
   u64 SavedCurveVersion;
   
   arena *Arena;
   u64 NumIterations;
   color *IterationColors;
   line_vertices *LineVerticesPerIteration;
   local_position *AllIntermediatePoints;
};

struct bezier_curve_degree_lowering
{
   b32 IsLowering;
   
   entity *CurveEntity;
   u64 SavedCurveVersion;
   
   arena *Arena;
   local_position *SavedControlPoints;
   f32 *SavedControlPointWeights;
   local_position *SavedCubicBezierPoints;
   u64 NumSavedCurveVertices;
   sf::Vertex *SavedCurveVertices;
   sf::PrimitiveType SavedPrimitiveType;
   
   bezier_lower_degree LowerDegree;
   f32 P_Mix;
   f32 W_Mix;
};

enum animate_curve_animation_stage
{
   AnimateCurveAnimation_None,
   AnimateCurveAnimation_PickingTarget,
   AnimateCurveAnimation_Animating,
};

enum animation_type
{
   Animation_Smooth,
   Animation_Linear,
   
   Animation_Count,
};

struct transform_curve_animation
{
   animate_curve_animation_stage Stage;
   
   entity *FromCurveEntity;
   entity *ToCurveEntity;
   
   arena *Arena;
   u64 SavedToCurveVersion;
   // NOTE(hbr): Number of points in ToCurve has to match number
   // of points in FromCurve in order to avoid animation artifacts.
   u64 NumCurvePoints;
   v2f32 *ToCurvePoints;
   line_vertices AnimatedCurveVertices;
   
   b32 IsAnimationPlaying;
   animation_type Animation;
   f32 AnimationSpeed;
   f32 AnimationMultiplier;
   f32 Blend;
};

enum curve_combination_type
{
   CurveCombination_Merge,
   
   CurveCombination_C0,
   CurveCombination_C1,
   CurveCombination_C2,
   
   CurveCombination_G1,
   
   CurveCombination_Count
};

struct curve_combining
{
   b32 IsCombining;
   
   entity *CombineCurveEntity;
   entity *TargetCurveEntity;
   
   b32 CombineCurveLastControlPoint;
   b32 TargetCurveFirstControlPoint;
   
   curve_combination_type CombinationType;
};

struct editor_state
{
   pool *EntityPool;
   entity *EntitiesHead;
   entity *EntitiesTail;
   u64 NumEntities;
   
   u64 EverIncreasingCurveCounter;
   
   entity *SelectedEntity;
   
   camera Camera;
   
   editor_mode Mode;
   
   splitting_bezier_curve SplittingBezierCurve;
   de_casteljau_visualization DeCasteljauVisualization;
   bezier_curve_degree_lowering DegreeLowering;
   transform_curve_animation CurveAnimation;
   arena *MovingPointArena;
   curve_combining CurveCombining;
};

internal editor_state CreateEditorState(pool *EntityPool, u64 CurveCounter, camera Camera,
                                        arena *DeCasteljauVisualizationArena, arena *DegreeLoweringArena,
                                        arena *MovingPointArena, arena *CurveAnimationArena,
                                        f32 CurveAnimationSpeed);

internal entity *AllocateAndAddEntity(editor_state *State);
internal void DeallocateAndRemoveEntity(editor_state *State, entity *Entity);

internal void SelectEntity(editor_state *EditorState, entity *Entity);
internal void DestroyEditorState(editor_state *EditorState);

enum save_project_format
{
   SaveProjectFormat_None,
   SaveProjectFormat_ProjectFile,
   SaveProjectFormat_ImageFile,
};

enum action_to_do
{
   ActionToDo_Nothing,
   ActionToDo_NewProject,
   ActionToDo_OpenProject,
   ActionToDo_Quit,
};

// NOTE(hbr): In seconds
#define NOTIFICATION_FADE_IN_TIME     0.15f
#define NOTIFICATION_FADE_OUT_TIME    0.15f
#define NOTIFICATION_PROPER_LIFE_TIME 10.0f
#define NOTIFICATION_INVISIBLE_TIME   0.1f

#define NOTIFICATION_TOTAL_LIFE_TIME \
(NOTIFICATION_FADE_OUT_TIME + \
NOTIFICATION_PROPER_LIFE_TIME + \
NOTIFICATION_FADE_OUT_TIME + \
NOTIFICATION_INVISIBLE_TIME)

enum notification_type
{
   Notification_Success,
   Notification_Error,
   Notification_Warning,
};

struct notification
{
   string Title;
   color TitleColor;
   
   string Text;
   
   f32 LifeTime;
   
   f32 PosY;
   f32 NotifWindowHeight;
};

struct notifications
{
   u64 NotifCount;
   notification Notifs[100];
   sf::RenderWindow *Window;
};

// NOTE(hbr): Not sure if this is the best name
struct ui_config
{
   b32 ViewSelectedEntityWindow;
   b32 ViewListOfEntitiesWindow;
   b32 ViewParametersWindow;
   b32 ViewDiagnosticsWindow;
   
   b32 DefaultCurveHeaderCollapsed;
   b32 RotationIndicatorHeaderCollapsed;
   b32 BezierSplitPointHeaderCollapsed;
   b32 OtherSettingsHeaderCollapsed;
   
   // TODO(hbr): Are those defines necessary? Tacke or remove TODO
#if EDITOR_PROFILER
   b32 ViewProfilerWindow;
#endif
   
#if BUILD_DEBUG
   b32 ViewDebugWindow;
#endif
};

struct editor
{
   sf::RenderWindow *Window;
   sf::Clock DeltaClock;
   
   frame_stats FrameStats;
   
   editor_state State;
   
   save_project_format SaveProjectFormat;
   string ProjectSavePath;
   
   action_to_do ActionWaitingToBeDone;
   
   projection Projection;
   
   notifications Notifications;
   
   editor_parameters Parameters;
   
   ui_config UI_Config;
};

#endif //EDITOR_EDITOR_H