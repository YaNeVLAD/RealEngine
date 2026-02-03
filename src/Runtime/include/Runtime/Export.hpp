#pragma once

#include <Core/Config.hpp>

#if defined(RE_RUNTIME_EXPORTS)

#define RE_RUNTIME_API RE_API_EXPORT

#else

#define RE_RUNTIME_API RE_API_IMPORT

#endif