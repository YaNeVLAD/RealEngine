#pragma once

#include <IgniLang/Semantic/SemanticType.hpp>

#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace igni::sem
{

struct Symbol
{
	re::String name;
	std::shared_ptr<SemanticType> type;
	bool isReadOnly;
};

class Environment
{
public:
	Environment()
	{
		PushScope(); // Global scope
	}

	void PushScope()
	{
		m_scopes.emplace_back();
	}

	void PopScope()
	{
		if (m_scopes.size() > 1)
		{
			m_scopes.pop_back();
		}
	}

	void Define(const re::String& name, const std::shared_ptr<SemanticType>& type, const bool isReadOnly = false)
	{
		if (m_scopes.back().contains(name))
		{
			throw std::runtime_error("Semantic Error: Variable '" + name + "' is already defined in this scope.");
		}
		m_scopes.back()[name] = { name, type, isReadOnly };
	}

	Symbol* Resolve(const re::String& name)
	{
		for (auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it)
		{
			if (it->contains(name))
			{
				return &(*it)[name];
			}
		}

		return nullptr;
	}

	[[nodiscard]] std::unordered_set<re::String> GetGlobalNames() const
	{
		std::unordered_set<re::String> names;
		if (!m_scopes.empty())
		{
			for (const auto& key : m_scopes.front() | std::views::keys)
			{
				names.insert(key);
			}
		}
		return names;
	}

private:
	std::vector<std::unordered_map<re::String, Symbol>> m_scopes;
};

} // namespace igni::sem