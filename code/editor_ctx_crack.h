#ifndef EDITOR_CTX_CRACK_H
#define EDITOR_CTX_CRACK_H

#ifdef _WIN32
# define OS_WINDOWS 1
#elif defined(__linux__)
# define OS_LINUX 1
#else
# error unsupported OS
#endif

#if defined(_MSC_VER)
# define COMPILER_MSVC 1
#elif defined(__GNUC__) || defined(__GNUG__)
# define COMPILER_GCC 1
#elif defined(__clang__)
# define COMPILER_CLANG 1
#else
# error unsupported compiler
#endif

#ifndef OS_WINDOWS
# define OS_WINDOWS 0
#endif
#ifndef OS_LINUX
# define OS_LINUX 0
#endif
#ifndef COMPILER_MSVC
# define COMPILER_MSVC 0
#endif
#ifndef COMPILER_GCC
# define COMPILER_GCC 0
#endif
#ifndef COMPILER_CLANG
# define COMPILER_CLANG 0
#endif

#endif //EDITOR_CTX_CRACK_H