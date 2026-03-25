#pragma once

#include <Core/String.hpp>

#include <memory>
#include <unordered_map>
#include <vector>

namespace igni::sem
{

struct SemanticType
{
	virtual ~SemanticType() = default;

	re::String name;

	virtual bool IsAssignableTo(const SemanticType* other) const
	{
		if (other->name.Hashed() == "Any"_hs)
		{
			return true;
		}

		return name == other->name;
	}
};

struct PrimitiveType final : SemanticType
{
	explicit PrimitiveType(const re::String& n) { name = n; }
};

struct ModuleType final : SemanticType
{
	std::unordered_map<re::String, std::shared_ptr<SemanticType>> exports;

	explicit ModuleType(const re::String& n) { name = n; }
};

struct FunctionType final : SemanticType
{
	std::vector<std::shared_ptr<SemanticType>> paramTypes;
	std::shared_ptr<SemanticType> returnType;
	bool isVararg = false;

	explicit FunctionType(const re::String& n) { name = n; }
};

struct ClassType final : SemanticType
{
	std::unordered_map<re::String, std::shared_ptr<SemanticType>> fields;
	std::unordered_map<re::String, std::shared_ptr<FunctionType>> methods;

	explicit ClassType(const re::String& n) { name = n; }
};

} // namespace igni::sem