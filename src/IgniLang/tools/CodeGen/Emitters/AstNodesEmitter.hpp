#pragma once

#include "../Builder.hpp"

#include <Core/HashedString.hpp>

#include <nlohmann/json.hpp>

namespace igni::codegen
{
using namespace re::literals;

class AstNodesEmitter
{
public:
	static constexpr auto TargetKey = "ast_nodes"_hs;

	static void Emit(const nlohmann::json& data, Builder<std::string>& h, Builder<std::string>& cpp)
	{
		auto ns = h.Namespace("igni::ast");

		for (const auto& node : data)
		{
			h.Line("struct " + node["name"].get<std::string>() + ";");
		}
		h.EmptyLine();

		h.Line("class IAstVisitor {");
		h.Line("public:");
		h.Line("    virtual ~IAstVisitor() = default;");
		for (const auto& node : data)
		{
			h.Line("    virtual void Visit(const " + node["name"].get<std::string>() + "* node) = 0;");
		}
		h.Line("};");
		h.EmptyLine();

		for (const auto& node : data)
		{
			auto name = node["name"].get<std::string>();
			auto base = node["base"].get<std::string>();

			h.Line("struct " + name + " final : public AstVisitable<" + name + ", " + base + "> {");

			if (node.contains("fields"))
			{
				for (const auto& field : node["fields"])
				{
					const auto fType = field["type"].get<std::string>();
					const auto fName = field["name"].get<std::string>();
					const bool isPtr = field.value("is_ptr", false);
					const bool isVector = field.value("is_vector", false);

					std::string cppType = fType;
					if (isPtr)
					{
						cppType = "std::unique_ptr<" + cppType + ">";
					}
					if (isVector)
					{
						cppType = "std::vector<" + cppType + ">";
					}

					if (auto defVal = field.value("default_val", ""); !defVal.empty())
					{
						h.Line("    " + cppType + " " + fName + " = " + defVal + ";");
					}
					else
					{
						h.Line("    " + cppType + " " + fName + ";");
					}
				}
			}
			h.Line("};");
			h.EmptyLine();
		}

		h.Line("class BaseAstVisitor : public IAstVisitor {");
		h.Line("public:");
		h.Line("    ~BaseAstVisitor() override = default;");
		for (const auto& node : data)
		{
			h.Line("    void Visit(const " + node["name"].get<std::string>() + "*) override {}");
		}
		h.Line("};");
	}
};

} // namespace igni::codegen