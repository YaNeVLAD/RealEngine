#pragma once

#include <Core/Config.hpp>

#if defined(RE_RVM_EXPORTS)

#define RE_RVM_API RE_API_EXPORT

#else

#define RE_RVM_API RE_API_IMPORT

#endif