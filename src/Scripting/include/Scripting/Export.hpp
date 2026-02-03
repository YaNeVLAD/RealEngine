#pragma once

#include <Core/Config.hpp>

#if defined(RE_SCRIPTING_EXPORTS)

#define RE_SCRIPTING_API RE_API_EXPORT

#else

#define RE_SCRIPTING_API RE_API_IMPORT

#endif