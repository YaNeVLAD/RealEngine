#pragma once

#include "../Builder.hpp"

#include <Core/HashedString.hpp>

#include <nlohmann/json.hpp>

namespace igni::codegen
{
using namespace re::literals;

class AstClonerEmitter
{
public:
	static constexpr auto TargetKey = "ast_nodes"_hs;

	static void Emit(const nlohmann::json& data, Builder<std::string>& h, Builder<std::string>& cpp)
	{
		auto nsH = h.Namespace("igni::ast::clone");
		auto nsCpp = cpp.Namespace("igni::ast::clone");

		// Добавим TypeEnv в сигнатуры!
		h.Line("std::unique_ptr<igni::ast::Node> CloneAst(const igni::ast::Node* root, const igni::ast::TypeEnv* env = nullptr);");
		h.EmptyLine();

		h.Line("template <typename T>");
		h.Line("std::unique_ptr<T> Clone(const T* node, const igni::ast::TypeEnv* env = nullptr) {");
		h.Line("    if (!node) return nullptr;");
		h.Line("    return std::unique_ptr<T>(static_cast<T*>(CloneAst(node, env).release()));");
		h.Line("}");
		h.EmptyLine();

		h.Line("template <typename T>");
		h.Line("std::vector<const T*> GetRawPointers(const std::vector<std::unique_ptr<T>>& in) {");
		h.Line("    std::vector<const T*> result;");
		h.Line("    result.reserve(in.size());");
		h.Line("    for (const auto& item : in) {");
		h.Line("        result.push_back(item.get());");
		h.Line("    }");
		h.Line("    return result;");
		h.Line("}");
		h.EmptyLine();

		// Объявление класса в HPP
		h.Line("class AstClonerVisitor : public igni::ast::BaseAstVisitor {");
		h.Line("    const igni::ast::TypeEnv* m_env = nullptr;");
		h.Line("public:");
		h.Line("    explicit AstClonerVisitor(const igni::ast::TypeEnv* env = nullptr) : m_env(env) {}");
		h.Line("    std::unique_ptr<igni::ast::Node> result;");
		h.EmptyLine();

		for (const auto& node : data)
		{
			const auto name = node["name"].get<std::string>();
			const auto base = node["base"].get<std::string>();

			h.Line("    void Visit(const igni::ast::" + name + "* node) override;");

			auto func = cpp.Function("void", "AstClonerVisitor::Visit", "const igni::ast::" + name + "* node");

			if (name == "SimpleTypeNode")
			{
				cpp.Line("if (m_env && m_env->contains(node->name)) {");
				cpp.Line("    auto substitutedType = Clone(m_env->at(node->name), m_env);");
				cpp.Line("    substitutedType->isNullable = node->isNullable || substitutedType->isNullable;");
				cpp.Line("    this->result = std::move(substitutedType);");
				cpp.Line("    return;");
				cpp.Line("}");
			}

			cpp.Line("auto clone = std::make_unique<igni::ast::" + name + ">();");
			cpp.Line("clone->token = node->token;");

			if (base == "Decl")
			{
				cpp.Line("clone->visibility = node->visibility;");
				cpp.Line("for (const auto& ann : node->annotations) { if(ann) { ann->Accept(*this); clone->annotations.push_back(std::unique_ptr<igni::ast::AnnotationNode>(static_cast<igni::ast::AnnotationNode*>(this->result.release()))); } }");
			}
			if (base == "TypeNode")
			{
				cpp.Line("clone->isNullable = node->isNullable;");
			}

			if (node.contains("fields"))
			{
				for (const auto& field : node["fields"])
				{
					auto fName = field["name"].get<std::string>();
					auto fType = field["type"].get<std::string>();
					const bool isPtr = field.value("is_ptr", false);
					const bool isVector = field.value("is_vector", false);

					if (isVector)
					{
						cpp.Line("for (const auto& item : node->" + fName + ") {");
						if (isPtr)
						{
							cpp.Line("    if (item) {");
							cpp.Line("        item->Accept(*this);");
							cpp.Line("        clone->" + fName + ".push_back(std::unique_ptr<igni::ast::" + fType + ">(static_cast<igni::ast::" + fType + "*>(this->result.release())));");
							cpp.Line("    } else { clone->" + fName + ".push_back(nullptr); }");
						}
						else
						{
							cpp.Line("    clone->" + fName + ".push_back(item);");
						}
						cpp.Line("}");
					}
					else if (isPtr)
					{
						cpp.Line("if (node->" + fName + ") {");
						cpp.Line("    node->" + fName + "->Accept(*this);");
						cpp.Line("    clone->" + fName + ".reset(static_cast<igni::ast::" + fType + "*>(this->result.release()));");
						cpp.Line("}");
					}
					else
					{
						cpp.Line("clone->" + fName + " = node->" + fName + ";");
					}
				}
			}
			cpp.Line("this->result = std::move(clone);");
		}
		h.Line("};");

		cpp.EmptyLine();
		auto exportFunc = cpp.Function("std::unique_ptr<igni::ast::Node>", "CloneAst", "const igni::ast::Node* root, const igni::ast::TypeEnv* env");
		cpp.Line("if (!root) return nullptr;");
		cpp.Line("AstClonerVisitor visitor(env);");
		cpp.Line("root->Accept(visitor);");
		cpp.Line("return std::move(visitor.result);");
	}
};

} // namespace igni::codegen