#pragma once

namespace igni::dotnet::SignatureUtils
{

inline re::String BuildTypeSignature(const std::vector<std::unique_ptr<ast::ParameterNode>>& params, const size_t startIndex, const bool isVararg)
{
	re::String sig;
	for (std::size_t i = startIndex; i < params.size(); ++i)
	{
		re::String typeSig = TypeMapper::MapAstType(params[i]->type.get());

		if (isVararg && i == params.size() - 1)
		{
			if (typeSig.Find("[]") == re::String::NPos)
			{
				typeSig += "[]";
			}
		}

		sig += typeSig;
		if (i < params.size() - 1)
		{
			sig += ", ";
		}
	}
	return sig;
}

inline re::String BuildTypeSignature(const std::vector<std::shared_ptr<sem::SemanticType>>& types, const size_t startIndex, const bool isVararg)
{
	re::String sig;
	for (std::size_t i = startIndex; i < types.size(); ++i)
	{
		re::String typeSig = TypeMapper::MapToCIL(types[i]->name);

		if (isVararg && i == types.size() - 1)
		{
			if (typeSig.Find("[]") == re::String::NPos)
			{
				typeSig += "[]";
			}
		}

		sig += typeSig;
		if (i < types.size() - 1)
		{
			sig += ", ";
		}
	}
	return sig;
}

inline re::String BuildCilSignature(const sem::FunctionType* funType, const re::String& ownerClass, const re::String& methodName, const bool isInstance, const bool skipFirstParam)
{
	const re::String retCilType = funType->returnType ? TypeMapper::MapToCIL(funType->returnType->name) : re::String("void");
	const re::String prefix = ownerClass.Empty() ? re::String("") : ownerClass + "::";

	re::String sig;
	if (isInstance)
	{
		sig += "instance ";
	}

	sig += retCilType;
	sig += " ";
	sig += prefix;
	sig += methodName;
	sig += "(";
	sig += BuildTypeSignature(funType->paramTypes, skipFirstParam ? 1 : 0, funType->isVararg);
	sig += ")";

	return sig;
}

inline re::String GetDelegateName(const sem::FunctionType* funType)
{
	std::string sigName = "Func";
	for (const auto& pt : funType->paramTypes)
	{
		auto pName = std::string(pt->name);
		std::ranges::replace(pName, '.', '_');
		std::ranges::replace(pName, '[', '_');
		std::ranges::replace(pName, ']', '_');
		sigName += "_" + pName;
	}

	auto rName = std::string(funType->returnType ? funType->returnType->name : "Unit");
	std::ranges::replace(rName, '.', '_');
	std::ranges::replace(rName, '[', '_');
	std::ranges::replace(rName, ']', '_');

	re::String finalName = sigName;
	finalName += "_Ret_";
	finalName += rName;

	return finalName;
}

} // namespace igni::dotnet::SignatureUtils