#ifndef BASE_CTX_CRACK_H
#define BASE_CTX_CRACK_H

#if defined(_WIN32)
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

#if !defined(OS_WINDOWS)
# define OS_WINDOWS 0
#endif
#if !defined(OS_LINUX)
# define OS_LINUX 0
#endif
#if !defined(COMPILER_MSVC)
# define COMPILER_MSVC 0
#endif
#if !defined(COMPILER_GCC)
# define COMPILER_GCC 0
#endif
#if !defined(COMPILER_CLANG)
# define COMPILER_CLANG 0
#endif
#if !defined(BUILD_DEBUG)
# define BUILD_DEBUG 0
#endif

#endif //BASE_CTX_CRACK_H
