#pragma once

#include <cstdint>

namespace re::core
{

using EntityID = uint32_t;
using TimeDelta = float;

enum class LogLevel
{
	Info,
	Warning,
	Error
};

} // namespace re::core