#pragma once

#include <Core/Config.hpp>

#if defined(IGNI_CLI_EXPORT)

#define IGNI_CLI_API RE_API_EXPORT

#else

#define IGNI_CLI_API RE_API_IMPORT

#endif