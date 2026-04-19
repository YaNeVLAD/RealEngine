#pragma once

#include <Core/Config.hpp>

#if defined(RE_GUI_EXPORTS)

#define RE_GUI_API RE_API_EXPORT

#else

#define RE_GUI_API RE_API_IMPORT

#endif