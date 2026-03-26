#pragma once

#include <Core/Config.hpp>

#if defined(IGNI_STD_EXPORT)

#define IGNI_STD_API extern "C" __declspec(dllexport)

#endif