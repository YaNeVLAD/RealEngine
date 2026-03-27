#pragma once

#include <Core/Config.hpp>

#if defined(RE_AUDIO_EXPORTS)

#define RE_AUDIO_API RE_API_EXPORT

#else

#define RE_AUDIO_API RE_API_IMPORT

#endif