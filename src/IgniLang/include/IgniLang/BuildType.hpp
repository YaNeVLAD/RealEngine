#pragma once

#include <cstdint>

namespace igni
{

enum class BuildType : std::int8_t
{
	Unknown = -1,

	Executable,
	DynamicLibrary,
};

} // namespace igni