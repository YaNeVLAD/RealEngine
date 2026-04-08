#pragma once

#include <Core/String.hpp>

#include <IgniLang/AST/AstNodes.hpp>

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
		if (name.Hashed() == "Any"_hs)
		{ // TODO: Remove after Generics and typecast
			return true;
		}

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
	bool isExternal = false;

	ast::Visibility visibility = ast::Visibility::Public;
	re::String moduleName;

	explicit FunctionType(const re::String& n) { name = n; }

	bool IsAssignableTo(const SemanticType* other) const override
	{
		if (name.Hashed() == "Any"_hs)
		{ // TODO: Remove after Generics and typecast
			return true;
		}

		if (other->name == "Any")
		{
			return true;
		}

		const auto* otherFun = dynamic_cast<const FunctionType*>(other);
		if (!otherFun)
		{
			return false;
		}

		if (!returnType->IsAssignableTo(otherFun->returnType.get()))
		{
			return false;
		}

		if (paramTypes.size() != otherFun->paramTypes.size())
		{
			return false;
		}

		for (std::size_t i = 0; i < paramTypes.size(); ++i)
		{
			if (!paramTypes[i]->IsAssignableTo(otherFun->paramTypes[i].get()))
			{
				return false;
			}
		}

		return true;
	}
};

struct ClassType final : SemanticType
{
	struct FieldInfo
	{
		std::shared_ptr<SemanticType> type;
		bool isReadOnly;
		ast::Visibility visibility = ast::Visibility::Public;
	};

	bool isExternal = false;

	std::unordered_map<re::String, FieldInfo> fields;
	std::unordered_map<re::String, std::shared_ptr<FunctionType>> methods;

	re::String moduleName;

	explicit ClassType(const re::String& n) { name = n; }
};

struct GenericFunctionTemplate final : SemanticType
{
	const ast::FunDecl* astNode{};
	std::vector<ast::GenericTypeParam> typeParams;
	re::String moduleName;

	explicit GenericFunctionTemplate(const re::String& n) { name = n; }
};

struct GenericClassTemplate final : SemanticType
{
	const ast::ClassDecl* astNode{};
	std::vector<ast::GenericTypeParam> typeParams;
	re::String moduleName;

	explicit GenericClassTemplate(const re::String& n) { name = n; }
};

} // namespace igni::sem