#pragma once

#if defined(_WIN32)
#if defined(CORE_EXPORTS)
#define RE_CORE_API __declspec(dllexport)
#else
#define RE_CORE_API __declspec(dllimport)
#endif
#else
#define RE_CORE_API
#endif