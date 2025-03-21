#ifndef PLATFORM_SHARED_H
#define PLATFORM_SHARED_H

#ifndef EDITOR_DLL
# error EDITOR_DLL with path to editor DLL code is not defined
#endif
#ifndef EDITOR_RENDERER_DLL
# error EDITOR_RENDERER_DLL with path to editor renderer DLL code is not defined
#endif
#define EDITOR_DLL_FILE_NAME ConvertNameToString(EDITOR_DLL)
#define EDITOR_RENDERER_DLL_FILE_NAME ConvertNameToString(EDITOR_RENDERER_DLL)

#endif //PLATFORM_SHARED_H
