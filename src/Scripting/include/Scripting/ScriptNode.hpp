#pragma once

#include <Core/Export.hpp>

#include <map>
#include <string>
#include <variant>
#include <vector>

#if defined(_WIN32)
#if defined(SCRIPTING_EXPORTS)
#define RE_SCRIPTING_API __declspec(dllexport)
#else
#define RE_SCRIPTING_API __declspec(dllimport)
#endif
#else
#define RE_SCRIPTING_API
#endif

namespace re::scripting
{

struct ScriptNode;

using ScriptValue = std::variant<int, float, std::string, bool>;
using ScriptChildren = std::vector<ScriptNode>;

// Структура, описывающая узел данных (например: army = { strength = 100 })
struct RE_SCRIPTING_API ScriptNode
{
	std::string m_key;
	std::variant<ScriptValue, ScriptChildren> m_value;

	bool IsLeaf() const;
	const ScriptValue& GetValue() const;
	const ScriptChildren& GetChildren() const;
};

class RE_SCRIPTING_API ScriptLoader
{
public:
	ScriptNode LoadFromFile(const std::string& path);
};

} // namespace re::scripting