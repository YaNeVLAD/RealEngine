#pragma once

#include <Core/String.hpp>
#include <Core/Utils.hpp>

#include <IgniLang/AST/AstNodes.hpp>

#include <memory>
#include <unordered_map>
#include <vector>

namespace igni::sem
{

struct SemanticType : re::utils::Prototype<SemanticType>
{
	re::String name;

	bool isNullable = false;

	virtual bool IsAssignableTo(const SemanticType* other) const
	{
		using namespace re::literals;

		if (name.Hashed() == "Any"_hs || other->name.Hashed() == "Any"_hs)
		{
			return true;
		}

		if (name.Hashed() == "Null"_hs)
		{
			return other->isNullable;
		}

		if (name != other->name)
		{
			return false;
		}

		if (isNullable && !other->isNullable)
		{
			return false;
		}

		return true;
	}
};

struct PrimitiveType final : re::utils::Clonable<PrimitiveType, SemanticType>
{
	explicit PrimitiveType(const re::String& n) { name = n; }
};

struct ModuleType final : re::utils::Clonable<ModuleType, SemanticType>
{
	std::unordered_map<re::String, std::shared_ptr<SemanticType>> exports;

	explicit ModuleType(const re::String& n) { name = n; }
};

struct FunctionType final : re::utils::Clonable<FunctionType, SemanticType>
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
		using namespace re::literals;

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

struct ClassType final : re::utils::Clonable<ClassType, SemanticType>
{
	struct FieldInfo
	{
		std::shared_ptr<SemanticType> type;
		bool isReadOnly;
		ast::Visibility visibility = ast::Visibility::Public;
	};

	bool isExternal = false;

	std::unordered_map<re::String, FieldInfo> fields;
	std::unordered_map<re::String, std::shared_ptr<SemanticType>> methods;

	re::String moduleName;

	std::vector<std::shared_ptr<SemanticType>> typeArguments;

	std::shared_ptr<ClassType> baseClass = nullptr;

	const ast::ClassDecl* classDecl = nullptr;

	explicit ClassType(const re::String& n) { name = n; }

	explicit ClassType(const re::String& n, const ast::ClassDecl* classDecl)
		: classDecl(classDecl)
	{
		name = n;
	}

	bool IsAssignableTo(const SemanticType* other) const override
	{
		if (SemanticType::IsAssignableTo(other))
		{
			return true;
		}

		const ClassType* current = baseClass.get();
		while (current != nullptr)
		{
			if (current->name == other->name)
			{
				return true;
			}
			current = current->baseClass.get();
		}

		return false;
	}
};

struct GenericFunctionTemplate final : re::utils::Clonable<GenericFunctionTemplate, SemanticType>
{
	const ast::FunDecl* astNode{};
	std::vector<ast::GenericTypeParam> typeParams;
	re::String moduleName;

	ast::Visibility visibility = ast::Visibility::Public;
	bool isExternal = false;

	explicit GenericFunctionTemplate(const re::String& n) { name = n; }
};

struct GenericClassTemplate final : re::utils::Clonable<GenericClassTemplate, SemanticType>
{
	const ast::ClassDecl* astNode{};
	std::vector<ast::GenericTypeParam> typeParams;
	re::String moduleName;

	explicit GenericClassTemplate(const re::String& n) { name = n; }
};

} // namespace igni::sem