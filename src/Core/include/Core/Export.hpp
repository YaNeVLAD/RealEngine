#pragma once

#include <Core/Config.hpp>

#if defined(RE_CORE_EXPORTS)

#define RE_CORE_API RE_API_EXPORT

#else

#define RE_CORE_API RE_API_IMPORT

#endif