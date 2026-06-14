#pragma once

#include <cstdint>

namespace igni
{

enum class BuildTarget : std::int8_t
{
	Unknown = -1,

	DotNet,
	RVM,
};

}