#pragma once

#include <Scripting/Export.hpp>

#include <string>

namespace re::scripting
{

class RE_SCRIPTING_API ScriptLoader
{
public:
	std::string LoadFromFile(const std::string& path);
};

} // namespace re::scripting