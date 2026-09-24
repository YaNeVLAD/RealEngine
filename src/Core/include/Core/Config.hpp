#pragma once

#if (defined(_MSVC_LANG) && _MSVC_LANG < 202100L) || (!defined(_MSVC_LANG) && __cplusplus < 202100L)
#error "Enable C++23 or newer for your compiler (e.g. -std=c++23 for GCC/Clang or /std:c++23 for MSVC)"
#endif

#if defined(_DEBUG) || !defined(NDEBUG)
#define RE_DEBUG
#endif

#if defined(_WIN32) && !defined(_WIN64)

#define RE_CALL __stdcall

#else

#define RE_CALL

#endif

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)

#define RE_SYSTEM_WINDOWS

#elif defined(__linux__)

#define RE_SYSTEM_LINUX
#define RE_SYSTEM_POSIX

#elif defined(__APPLE__)

#define RE_SYSTEM_MACOS
#define RE_SYSTEM_POSIX

#else

#error This OS is not supported by RealEngine

#endif

#if defined(RE_SYSTEM_WINDOWS)

#define RE_DYNAMIC_EXPORT __declspec(dllexport)
#define RE_DYNAMIC_IMPORT __declspec(dllimport)

#else

#define RE_DYNAMIC_EXPORT __attribute__((__visibility__("default")))
#define RE_DYNAMIC_IMPORT __attribute__((__visibility__("default")))

#endif

#if defined(RE_STATIC)

#define RE_API_EXPORT
#define RE_API_IMPORT

#else

#define RE_API_EXPORT RE_DYNAMIC_EXPORT
#define RE_API_IMPORT RE_DYNAMIC_IMPORT

#if defined(_MSC_VER)

#pragma warning(disable : 4251) // Using standard library types in our own exported types is okay
#pragma warning(disable : 4275) // Exporting types derived from the standard library is okay

#endif

#endif