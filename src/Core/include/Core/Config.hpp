#pragma once

#if (defined(_MSVC_LANG) && _MSVC_LANG < 202100L) || (!defined(_MSVC_LANG) && __cplusplus < 202100L)
#error "Enable C++23 or newer for your compiler (e.g. -std=c++23 for GCC/Clang or /std:c++23 for MSVC)"
#endif

#if defined(_WIN32)

#define RE_SYSTEM_WINDOWS

#else

#error This operating system is not supported by Real Engine

#endif

#if !defined(RE_STATIC)

#if defined(RE_SYSTEM_WINDOWS)

#define RE_API_EXPORT __declspec(dllexport)
#define RE_API_IMPORT __declspec(dllimport)

#ifdef _MSC_VER

#pragma warning(disable : 4251) // Using standard library types in our own exported types is okay
#pragma warning(disable : 4275) // Exporting types derived from the standard library is okay

#endif

#else

#define RE_API_EXPORT __attribute__((__visibility__("default")))
#define RE_API_IMPORT __attribute__((__visibility__("default")))

#endif

#else

#define RE_API_EXPORT
#define RE_API_IMPORT

#endif