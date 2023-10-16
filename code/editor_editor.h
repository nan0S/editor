#ifndef EDITOR_EDITOR_H
#define EDITOR_EDITOR_H

//- Parameters and constants
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

function editor_parameters EditorParametersMake(f32 RotationIndicatorRadiusClipSpace,
                                                f32 RotationIndicatorOutlineThicknessFraction,
                                                color RotationIndicatorFillColor,
                                                color RotationIndicatorOutlineColor,
                                                f32 BezierSplitPointRadiusClipSpace,
                                                f32 BezierSplitPointOutlineThicknessFraction,
                                                color BezierSplitPointFillColor,
                                                color BezierSplitPointOutlineColor,
                                                color BackgroundColor,
                                                f32 CollisionToleranceClipSpace,
                                                f32 LastControlPointSizeMultiplier,
                                                f32 SelectedCurveControlPointOutlineThicknessScale,
                                                color SelectedCurveControlPointOutlineColor,
                                                color SelectedControlPointOutlineColor,
                                                f32 CubicBezierHelperLineWidthClipSpace,
                                                curve_params CurveDefaultParams,
                                                f32 DefaultControlPointWeight);

//- Camera, Projection and Coordinate Spaces
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
   sf::RenderWindow *RenderWindow;
   camera Camera;
   projection Projection;
};

function camera CameraMake(v2f32 Position, rotation_2d Rotation,
                           f32 Zoom, f32 ZoomSensitivity, f32 ReachingTargetSpeed);
function void CameraUpdate(camera *Camera, f32 MouseWheelDelta, f32 DeltaTime);
function void CameraMove(camera *Camera, v2f32 Translation);
function void CameraRotate(camera *Camera, rotation_2d Rotation);
function void CameraSetZoom(camera *Camera, f32 Zoom);

function projection ProjectionMake(u64 Width, u64 Height, f32 FrustumSize);
function void ProjectionOnWindowResize(projection *Projection, u64 NewWidth, u64 NewHeight);

function coordinate_system_data CoordinateSystemDataMake(sf::RenderWindow *RenderWindow,
                                                         camera Camera, projection Projection);

function camera_position ScreenToCameraSpace(screen_position Position, coordinate_system_data Data);
function world_position  CameraToWorldSpace (camera_position Position, coordinate_system_data Data);
function camera_position WorldToCameraSpace (world_position Position,  coordinate_system_data Data);
function world_position  ScreenToWorldSpace (screen_position Position, coordinate_system_data Data);

// NOTE(hbr): Distance in space [-AspectRatio, AspectRatio] x [-1, 1]
function f32 ClipSpaceLengthToWorldSpace(f32 ClipSpaceDistance, coordinate_system_data Data);

//- Collisions
enum collision_type
{
   Collision_None,
   Collision_ControlPoint,
   Collision_CurvePoint,
   Collision_Image,
};

struct collision
{
   collision_type Type;
   
   // NOTE(hbr): Fat-struct to reduce complexity
   curve *CollisionCurve;
   u64 CollisionPointIndex;
   b32 IsCubicBezierPoint;
   
   image *CollisionImage;
};

typedef u64 check_collision_with_flags;
enum
{
   CheckCollisionWith_ControlPoints     = (1<<0),
   CheckCollisionWith_CurvePoints       = (1<<1),
   CheckCollisionWith_Images            = (1<<2),
};

function collision ControlPointCollision(curve *CollisionCurve, u64 CollisionPointIndex, b32 IsCubicBezierPoint);
function collision CurvePointCollision(curve *CollisionCurve, u64 CollisionCurvePointIndex);
function collision ImageCollision(image *CollisionImage);

// TODO(hbr): Remove, not used anymore
typedef u64 curve_point_index;
function curve_point_index CheckCollisionWithCurvePoints(curve *Curve,
                                                         world_position CheckPosition,
                                                         f32 CollisionTolerance);
function collision CheckCollisionWith(curve *CurveListTail, image *ImageList,
                                      world_position CheckPosition,
                                      f32 CollisionTolerance,
                                      editor_parameters EditorParams,
                                      check_collision_with_flags CheckCollisionWithFlags);

//- Editor
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

function user_action UserActionButtonClicked(button Button, screen_position ClickPosition, user_input *UserInput);
function user_action UserActionButtonDrag(button Button, screen_position DragFromPosition, user_input *UserInput);
function user_action UserActionButtonReleased(button Button, screen_position ReleasePosition, user_input *UserInput);
function user_action UserActionMouseMove(v2s32 FromPosition, screen_position ToPosition, user_input *UserInput);

enum editor_mode_type
{
   EditorMode_Normal,
   EditorMode_MovingPoint,
   EditorMode_MovingCurve,
   EditorMode_MovingImage,
   EditorMode_MovingCamera,
   EditorMode_RotatingCurve,
   EditorMode_RotatingImage,
   EditorMode_RotatingCamera,
};

struct editor_mode
{
   editor_mode_type Type;
   union
   {
      struct {
         curve *Curve;
         u64 PointIndex;
         b32 CubicBezierPointMoving;
         
         sf::Vertex *SavedCurveVertices;
         u64 SavedNumCurveVertices;
         sf::PrimitiveType SavedPrimitiveType;
      } MovingPoint;
      
      curve *MovingCurve;
      
      image *MovingImage;
      
      struct {
         curve *Curve;
         screen_position RotationCenter;
      } RotatingCurve;
      
      image *RotatingImage;
   };
};

function editor_mode EditorModeNormal(void);
function editor_mode EditorModeMovingPoint(curve *Curve, u64 PointIndex, b32 CubicBezierPointMoving, arena *Arena);
function editor_mode EditorModeMovingCurve(curve *Curve);
function editor_mode EditorModeMovingImage(image *Image);
function editor_mode EditorModeMovingCamera(void);
function editor_mode EditorModeRotatingCurve(curve *Curve, screen_position RotationCenter);
function editor_mode EditorModeRotatingImage(image *Image);
function editor_mode EditorModeRotatingCamera(void);

enum selected_entity_type
{
   SelectedEntity_None,
   SelectedEntity_Curve,
   SelectedEntity_Image,
};

struct splitting_bezier_curve
{
   b32 IsSplitting;
   curve *SplitCurve;
   f32 T;
   b32 DraggingSplitPoint;
   u64 SavedCurveVersion;
   world_position SplitPoint;
};

struct de_casteljau_visualization
{
   b32 IsVisualizing;
   f32 T;
   
   curve *Curve;
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
   
   curve *Curve;
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
   
   curve *FromCurve;
   curve *ToCurve;
   
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

function f32 CalculateAnimation(animation_type Animation, f32 T);
function char const *AnimationToString(animation_type Animation);

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
   
   curve *CombineCurve;
   curve *TargetCurve;
   
   b32 CombineCurveLastControlPoint;
   b32 TargetCurveFirstControlPoint;
   
   curve_combination_type CombinationType;
};

struct editor_state
{
   pool *CurvePool;
   curve *CurvesHead;
   curve *CurvesTail;
   u64 NumCurves;
   
   u64 EverIncreasingCurveCounter;
   
   pool *ImagePool;
   image *ImagesHead;
   image *ImagesTail;
   u64 NumImages;
   
   selected_entity_type SelectedEntityType;
   union {
      curve *Curve;
      image *Image;
   } SelectedEntity;
   
   camera Camera;
   
   editor_mode Mode;
   
   splitting_bezier_curve SplittingBezierCurve;
   de_casteljau_visualization DeCasteljauVisualization;
   bezier_curve_degree_lowering DegreeLowering;
   transform_curve_animation CurveAnimation;
   arena *MovingPointArena;
   curve_combining CurveCombining;
};

function editor_state EditorStateMake(pool *CurvePool, pool *ImagePool, u64 CurveCounter, camera Camera,
                                      arena *DeCasteljauVisualizationArena, arena *DegreeLoweringArena,
                                      arena *MovingPointArena, arena *CurveAnimationArena,
                                      f32 CurveAnimationSpeed);
function editor_state EditorStateMakeDefault(pool *CurvePool, pool *ImagePool, arena *DeCasteljauVisualizationArena,
                                             arena *DegreeLoweringArena, arena *MovingPointArena, arena *CurveAnimationArena);
function void         EditorStateDestroy(editor_state *EditorState);
function curve *      EditorStateAddCurve(editor_state *EditorState, curve Curve);
function image *      EditorStateAddImage(editor_state *EditorState, image Image);
function void         EditorStateRemoveCurve(editor_state *EditorState, curve *Curve);
function void         EditorStateRemoveImage(editor_state *EditorState, image *RemoveImage);
function void         EditorStateSelectCurve(editor_state *EditorState, curve *Curve,
                                             u64 SelectedControlPointIndex);
function void         EditorStateSelectImage(editor_state *EditorState, image *Image);
function void         EditorStateDeselectCurrentEntity(editor_state *EditorState);

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
   
   f32 PositionY;
   f32 NotificationWindowHeight;
};

struct notification_system
{
   u64 NumNotifications;
   notification Notifications[100];
   
   sf::RenderWindow *Window;
};

function notification NotificationMake(string Title,
                                       color TitleColor,
                                       string Text,
                                       f32 PositionY);
function void NotificationDestroy(notification *Notification);

function notification_system NotificationSystemMake(sf::RenderWindow *Window);
function void NotificationSystemAddNotificationFormat(notification_system *NotificationSystem,
                                                      notification_type NotificationType,
                                                      char const *NotificationTextFormat,
                                                      ...);

// NOTE(hbr): Not sure if this is the best name
struct ui_config
{
   b32 ViewSelectedCurveWindow;
   b32 ViewSelectedImageWindow;
   
   b32 ViewListOfEntitiesWindow;
   b32 ViewParametersWindow;
   b32 ViewDiagnosticsWindow;
   
   b32 DefaultCurveHeaderCollapsed;
   b32 RotationIndicatorHeaderCollapsed;
   b32 BezierSplitPointHeaderCollapsed;
   b32 OtherSettingsHeaderCollapsed;
   
#if EDITOR_PROFILER
   b32 ViewProfilerWindow;
#endif
   
#if EDITOR_DEBUG
   b32 ViewDebugWindow;
#endif
};

function ui_config UI_ConfigMake(b32 ViewSelectedCurveWindow,
                                 b32 ViewSelectedImageWindow,
                                 b32 ViewListOfEntitiesWindow,
                                 b32 ViewParametersWindow,
                                 b32 ViewDiagnosticsWindow,
                                 b32 DefaultCurveHeaderCollapsed,
                                 b32 RotationIndicatorHeaderCollapsed,
                                 b32 BezierSplitPointHeaderCollapsed,
                                 b32 OtherSettingsHeaderCollapsed);

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
   
   notification_system NotificationSystem;
   
   editor_parameters Parameters;
   
   ui_config UI_Config;
};

function editor EditorMake(sf::RenderWindow *Window,
                           editor_state EditorState,
                           projection Projection,
                           notification_system NotificationSystem,
                           editor_parameters EditorParameters,
                           ui_config UI_Config);
function void EditorSetSaveProjectPath(editor *Editor,
                                       save_project_format SaveProjectFormat,
                                       string SaveProjectFilePath);

function void UpdateAndRender(f32 DeltaTime, user_input UserInput, editor *Editor);

#endif //EDITOR_EDITOR_H