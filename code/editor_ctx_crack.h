#ifndef EDITOR_CTX_CRACK_H
#define EDITOR_CTX_CRACK_H

////////////////////////////////
//~ hbr: MSVC Extraction

#if defined(_MSC_VER)

# if defined(_WIN32)
#  define OS_WINDOWS 1
# else
#  error _MSC_VER is defined, but _WIN32 is not. This setup is not supported.
# endif

////////////////////////////////
//~ hbr: Clang Extraction

#elif defined(__clang__)

# if defined(__APPLE__) && defined(__MACH__)
#  define OS_MAC 1
# elif defined(__gnu_linux__)
#  define OS_LINUX 1
# else
#  error __clang__ is defined, but one of {__APPLE__, __gnu_linux__} is not. This setup is not supported.
# endif

////////////////////////////////
//~ hbr: GCC Extraction

#elif defined(__GNUC__) || defined(__GNUG__)

# if defined(__gnu_linux__)
#  define OS_LINUX 1
# else
#  error __GNUC__ or __GNUG__ is defined, but __gnu_linux__ is not. This setup is not supported.
# endif

#else
# error Compiler is not supported. _MSC_VER, __clang__, __GNUC__, or __GNUG__ must be defined.
#endif

////////////////////////////////
//~ hbr: Zero
#if !defined(OS_WINDOWS)
#define OS_WINDOWS 0
#endif
#if !defined(OS_LINUX)
#define OS_LINUX 0
#endif
#if !defined(OS_MAC)
#define OS_MAC 0
#endif

#endif //EDITOR_CTX_CRACK_H