#pragma once

#if defined(_WIN32)
#if defined(RUNTIME_EXPORTS)
#define RE_RUNTIME_API __declspec(dllexport)
#else
#define RE_RUNTIME_API __declspec(dllimport)
#endif
#else
#define RE_RUNTIME_API
#endif