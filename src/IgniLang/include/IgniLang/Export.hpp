#pragma once

#include <Core/Config.hpp>

#if defined(IGNI_EXPORT)

#define IGNI_API RE_API_EXPORT

#else

#define IGNI_API RE_API_IMPORT

#endif