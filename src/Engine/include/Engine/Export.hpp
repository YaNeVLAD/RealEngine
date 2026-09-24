#pragma once

#include <Core/Config.hpp>

#if defined(RE_ENGINE_EXPORTS)

#define RE_ENGINE_API RE_DYNAMIC_EXPORT

#else

#define RE_ENGINE_API RE_DYNAMIC_IMPORT

#endif