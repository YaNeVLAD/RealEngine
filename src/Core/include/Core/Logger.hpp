#pragma once

#include <Core/Export.hpp>

namespace re::core
{

enum class LogLevel
{
	Info,
	Warning,
	Error
};

class RE_CORE_API Logger
{
public:
	Logger();
};

}