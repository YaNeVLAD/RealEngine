#pragma once

#include "../Builder.hpp"
#include <Core/HashedString.hpp>
#include <nlohmann/json.hpp>

namespace igni::codegen
{
using namespace re::literals;

class AstSerializerEmitter
{
public:
	static constexpr auto TargetKey = "ast_nodes"_hs;

	static void Emit(const nlohmann::json& data, Builder<std::string>& h, Builder<std::string>& cpp)
	{
		auto nsH = h.Namespace("igni::ast::json");
		auto nsCpp = cpp.Namespace("igni::ast::json");

		h.Line("nlohmann::json SerializeAst(const igni::ast::Node* root);");

		cpp.Line("class JsonSerializerVisitor : public igni::ast::BaseAstVisitor {");
		cpp.Line("public:");
		cpp.Line("    nlohmann::json result;");
		cpp.EmptyLine();

		for (const auto& node : data)
		{
			const auto name = node["name"].get<std::string>();
			const auto base = node["base"].get<std::string>();

			const auto func = cpp.Function("void", "Visit", "const igni::ast::" + name + "* node", "override");

			cpp.Line(R"(result["type"] = ")" + name + "\";");

			if (base == "Decl")
			{
				cpp.Line("result[\"visibility\"] = static_cast<int>(node->visibility);");
				cpp.Line("result[\"annotations\"] = nlohmann::json::array();");
				cpp.Line("for (const auto& ann : node->annotations) { if(ann) { JsonSerializerVisitor sub; ann->Accept(sub); result[\"annotations\"].push_back(sub.result); } }");
			}
			if (base == "TypeNode")
			{
				cpp.Line("result[\"isNullable\"] = node->isNullable;");
			}

			if (node.contains("fields"))
			{
				for (const auto& field : node["fields"])
				{
					const auto fName = field["name"].get<std::string>();
					const bool isPtr = field.value("is_ptr", false);
					const bool isVector = field.value("is_vector", false);

					if (isVector)
					{
						cpp.Line("result[\"" + fName + "\"] = nlohmann::json::array();");
						cpp.Line("for (const auto& item : node->" + fName + ") {");
						if (isPtr)
						{
							cpp.Line("    if (item) {");
							cpp.Line("        JsonSerializerVisitor sub; item->Accept(sub);");
							cpp.Line("        result[\"" + fName + "\"].push_back(sub.result);");
							cpp.Line("    } else { result[\"" + fName + "\"].push_back(nullptr); }");
						}
						else
						{
							cpp.Line("    result[\"" + fName + "\"].push_back(item);"); // Если это не указатели
						}
						cpp.Line("}");
					}
					else if (isPtr)
					{
						cpp.Line("if (node->" + fName + ") {");
						cpp.Line("    JsonSerializerVisitor sub; node->" + fName + "->Accept(sub);");
						cpp.Line("    result[\"" + fName + "\"] = sub.result;");
						cpp.Line("} else { result[\"" + fName + "\"] = nullptr; }");
					}
					else
					{
						cpp.Line("result[\"" + fName + "\"] = node->" + fName + ";");
					}
				}
			}
		}
		cpp.Line("};");

		cpp.EmptyLine();
		auto exportFunc = cpp.Function("nlohmann::json", "SerializeAst", "const igni::ast::Node* root");
		cpp.Line("if (!root) return nullptr;");
		cpp.Line("JsonSerializerVisitor visitor;");
		cpp.Line("root->Accept(visitor);");
		cpp.Line("return visitor.result;");
	}
};

} // namespace igni::codegen