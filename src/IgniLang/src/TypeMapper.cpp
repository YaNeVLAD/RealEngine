#include <IgniLang/Compiler/DotNet/TypeMapper.hpp>

#include <Core/flat_map.hpp>

namespace
{

template <std::size_t N>
using HashedStringMap = re::flat_map<re::HashedString, std::string_view, N>;

constexpr auto TYPE_OBJECT = "class [mscorlib]System.Object";
constexpr auto TYPE_VOID = "void";
constexpr auto ANNO_BASE_CLASS = "DotNetBaseClass";

} // namespace

namespace igni::dotnet
{

re::String TypeMapper::MapToCIL(const re::String& semTypeName)
{
	using namespace re::literals;

	static constexpr HashedStringMap SemTypeMap = { {
		{ "System.Int64"_hs, "int64" },
		{ "System.Double"_hs, "float64" },
		{ "System.String"_hs, "string" },
		{ "System.Boolean"_hs, "bool" },
		{ "Unit"_hs, TYPE_VOID },
		{ "System.Void"_hs, TYPE_VOID },
		{ "Null"_hs, TYPE_OBJECT },
		{ "Any"_hs, TYPE_OBJECT },
		{ "System.Object"_hs, TYPE_OBJECT },
	} };

	if (const auto mapped = SemTypeMap[semTypeName.Hashed()])
	{
		return mapped->data();
	}

	if (semTypeName.Find("Array@") != re::String::NPos)
	{
		const re::String innerType = semTypeName.Substring(6, semTypeName.Length() - 6);
		return MapToCIL(innerType) + "[]";
	}

	return "class " + semTypeName;
}

re::String TypeMapper::MapAstType(const ast::TypeNode* node)
{
	using namespace re::literals;

	if (const auto s = dynamic_cast<const ast::SimpleTypeNode*>(node))
	{
		if (s->name == "Array" && !s->typeArgs.empty())
		{
			return MapAstType(s->typeArgs[0].get()) + "[]";
		}

		static constexpr HashedStringMap AstTypeMap = { {
			{ "Int"_hs, "int64" },
			{ "Double"_hs, "float64" },
			{ "String"_hs, "string" },
			{ "Bool"_hs, "bool" },
			{ "Unit"_hs, TYPE_VOID },
			{ "Any"_hs, TYPE_OBJECT },
		} };

		if (const auto mapped = AstTypeMap[s->name.Hashed()])
		{
			return mapped->data();
		}

		return MapToCIL(s->name);
	}

	return TYPE_OBJECT;
}

re::String TypeMapper::GetElemSuffix(const re::String& semTypeName)
{
	using namespace re::literals;

	static constexpr HashedStringMap SemTypeMap = { {
		{ "System.Int64"_hs, "i8" },
		{ "System.Double"_hs, "r8" },
		{ "System.Boolean"_hs, "i4" },
		{ "int64"_hs, "i8" },
		{ "float64"_hs, "r8" },
		{ "bool"_hs, "i4" },
	} };

	return SemTypeMap.get(semTypeName.Hashed(), "ref");
}

std::string TypeMapper::BuildParamSignature(const std::vector<std::unique_ptr<ast::ParameterNode>>& params, const bool isVararg)
{
	re::String sig;
	for (std::size_t i = 0; i < params.size(); ++i)
	{
		auto typeSig = MapAstType(params[i]->type.get());
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

} // namespace igni::dotnet