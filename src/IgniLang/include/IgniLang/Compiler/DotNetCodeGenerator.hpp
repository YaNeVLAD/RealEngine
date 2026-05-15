#pragma once

#include <IgniLang/AST/AstNodes.hpp>
#include <IgniLang/Semantic/SemanticAnalyzer.hpp>

#include <ostream>

namespace igni
{

class DotNetCodeGenerator final : public ast::BaseAstVisitor
{
public:
	DotNetCodeGenerator(std::ostream& out, const sem::SemanticAnalyzer& semantics)
		: m_out(out)
		, m_semanticAnalyzer(semantics)
	{
	}

	void Generate(const ast::Program* program)
	{
		m_out << "// Auto-generated .NET CIL\n";
		m_out << ".assembly IgniProgram { }\n";
		m_out << ".assembly extern mscorlib { }\n\n";

		m_out << ".class public auto ansi beforefieldinit Program extends [mscorlib]System.Object\n{\n";

		for (const auto& stmt : program->statements)
		{
			if (stmt)
			{
				stmt->Accept(*this);
			}
		}

		m_out << "}\n";
	}

	void Visit(const ast::FunDecl* node) override
	{
		if (node->isExternal)
		{
			return;
		}

		m_locals.clear();
		m_localTypes.clear();

		std::stringstream bodyStream;
		std::ostream* oldOut = &m_out;
		m_currentOut = &bodyStream;

		if (node->body)
		{
			node->body->Accept(*this);
		}

		m_currentOut = oldOut;

		re::String retType = "void";
		if (node->returnType)
		{
			const auto semType = dynamic_cast<const ast::SimpleTypeNode*>(node->returnType.get());
			if (semType)
			{
				retType = MapTypeToCIL(semType->name);
			}
		}

		m_out << "  .method public static " << retType << " " << node->name << "() cil managed\n  {\n";
		if (node->name == "main")
		{
			m_out << "    .entrypoint\n";
		}
		m_out << "    .maxstack 8\n";

		if (!m_locals.empty())
		{
			m_out << "    .locals init (\n";
			for (size_t i = 0; i < m_locals.size(); ++i)
			{
				m_out << "      [" << i << "] " << m_localTypes[i] << " '" << m_locals[i] << "'" << (i == m_locals.size() - 1 ? "" : ",") << "\n";
			}
			m_out << "    )\n";
		}

		m_out << bodyStream.str();

		m_out << "    ret\n";
		m_out << "  }\n\n";
	}

	void Visit(const ast::Block* node) override
	{
		for (const auto& s : node->statements)
		{
			if (s)
			{
				s->Accept(*this);
			}
		}
	}

	void Visit(const ast::VarDecl* node) override
	{
		DeclareLocal(node->name, node->type.get(), node->initializer.get());
	}

	void Visit(const ast::ValDecl* node) override
	{
		DeclareLocal(node->name, node->type.get(), node->initializer.get());
	}

	void Visit(const ast::AssignExpr* node) override
	{
		if (node->value)
		{
			node->value->Accept(*this);
		}

		if (const auto id = dynamic_cast<const ast::IdentifierExpr*>(node->target.get()))
		{
			if (const int idx = GetLocalIndex(id->name); idx != -1)
			{
				*m_currentOut << "    stloc." << idx << " // " << id->name << "\n";
			}
		}
	}

	void Visit(const ast::IdentifierExpr* node) override
	{
		if (const int idx = GetLocalIndex(node->name); idx != -1)
		{
			*m_currentOut << "    ldloc." << idx << " // " << node->name << "\n";
		}
	}

	void Visit(const ast::LiteralExpr* node) override
	{
		if (node->token.type == TokenType::IntConst)
		{
			*m_currentOut << "    ldc.i8 " << node->token.lexeme << "\n";
		}
		else if (node->token.type == TokenType::FloatConst)
		{
			*m_currentOut << "    ldc.r8 " << node->token.lexeme << "\n";
		}
		else if (node->token.type == TokenType::StringConst)
		{
			*m_currentOut << "    ldstr " << node->token.lexeme << "\n";
		}
	}

	void Visit(const ast::BinaryExpr* node) override
	{
		using namespace re::literals;
		if (node->left)
		{
			node->left->Accept(*this);
		}
		if (node->right)
		{
			node->right->Accept(*this);
		}

		switch (node->op.Hashed())
		{ // clang-format off
		case "+"_hs:  *m_currentOut << "    add\n"; break;
		case "-"_hs:  *m_currentOut << "    sub\n"; break;
		case "*"_hs:  *m_currentOut << "    mul\n"; break;
		case "/"_hs:  *m_currentOut << "    div\n"; break;
		case "%"_hs:  *m_currentOut << "    rem\n"; break;

		case "=="_hs: *m_currentOut << "    ceq\n"; break;
		case ">"_hs:  *m_currentOut << "    cgt\n"; break;
		case "<"_hs:  *m_currentOut << "    clt\n"; break;

		case "!="_hs: *m_currentOut << "    ceq\n    ldc.i4.0\n    ceq\n"; break;
		case "<="_hs: *m_currentOut << "    cgt\n    ldc.i4.0\n    ceq\n"; break;
		case ">="_hs: *m_currentOut << "    clt\n    ldc.i4.0\n    ceq\n"; break;
		default: break;
		} // clang-format on
	}

	void Visit(const ast::IfStmt* node) override
	{
		const std::size_t currentLabel = m_labelCount++;
		const std::string elseLabel = "L_else_" + std::to_string(currentLabel);
		const std::string endLabel = "L_end_" + std::to_string(currentLabel);

		if (node->condition)
		{
			node->condition->Accept(*this);
		}

		*m_currentOut << "    brfalse " << (node->elseBranch ? elseLabel : endLabel) << "\n";

		if (node->thenBranch)
			node->thenBranch->Accept(*this);

		if (node->elseBranch)
		{
			*m_currentOut << "    br " << endLabel << "\n";
			*m_currentOut << elseLabel << ":\n";
			node->elseBranch->Accept(*this);
		}

		*m_currentOut << endLabel << ":\n";
	}

	void Visit(const ast::WhileStmt* node) override
	{
		const std::size_t currentLabel = m_labelCount++;
		const std::string startLabel = "L_while_start_" + std::to_string(currentLabel);
		const std::string endLabel = "L_while_end_" + std::to_string(currentLabel);

		*m_currentOut << startLabel << ":\n";

		if (node->condition)
		{
			node->condition->Accept(*this);
		}
		*m_currentOut << "    brfalse " << endLabel << "\n";

		if (node->body)
		{
			node->body->Accept(*this);
		}

		*m_currentOut << "    br " << startLabel << "\n";
		*m_currentOut << endLabel << ":\n";
	}

	void Visit(const ast::ForStmt* node) override
	{
		if (node->isForEach)
		{
			return; // TODO: Add Array iteration
		}

		const std::size_t currentLabel = m_labelCount++;
		const std::string startLabel = "L_for_start_" + std::to_string(currentLabel);
		const std::string endLabel = "L_for_end_" + std::to_string(currentLabel);

		DeclareLocal(node->iteratorName, nullptr, node->startExpr.get());
		const int iterIdx = GetLocalIndex(node->iteratorName);

		const std::string limitName = "_for_limit_" + std::to_string(currentLabel);
		DeclareLocal(limitName, nullptr, node->endExpr.get());
		const int limitIdx = GetLocalIndex(limitName);

		*m_currentOut << startLabel << ":\n";

		*m_currentOut << "    ldloc." << iterIdx << "\n";
		*m_currentOut << "    ldloc." << limitIdx << "\n";
		*m_currentOut << "    bgt " << endLabel << "\n";

		if (node->body)
		{
			node->body->Accept(*this);
		}

		*m_currentOut << "    ldloc." << iterIdx << "\n";
		*m_currentOut << "    ldc.i8 1\n";
		*m_currentOut << "    add\n";
		*m_currentOut << "    stloc." << iterIdx << "\n";

		*m_currentOut << "    br " << startLabel << "\n";
		*m_currentOut << endLabel << ":\n";
	}

	void Visit(const ast::CallExpr* node) override
	{
		const auto& callInfo = m_semanticAnalyzer.GetBindings().callInfo.at(node);

		for (const auto& arg : node->arguments)
		{
			if (arg)
			{
				arg->Accept(*this);
			}
		}

		if (callInfo.dispatchMode == CallDispatchType::Native)
		{
			*m_currentOut << "    call " << callInfo.asmLabel << "\n";
		}
		else
		{
			re::String retType = "void";
			if (callInfo.target && callInfo.target->returnType)
			{
				retType = MapTypeToCIL(callInfo.target->returnType->name);
			}
			*m_currentOut << "    call " << retType << " Program::" << callInfo.asmLabel << "()\n";
		}
	}

	void Visit(const ast::ExprStmt* node) override
	{
		if (node->expr)
		{
			node->expr->Accept(*this);
		}
	}

private:
	std::ostream& m_out;
	std::ostream* m_currentOut = &m_out;
	const sem::SemanticAnalyzer& m_semanticAnalyzer;

	std::vector<re::String> m_locals;
	std::vector<re::String> m_localTypes;

	std::size_t m_labelCount = 0;

	static re::String MapTypeToCIL(const re::String& semTypeName)
	{
		using namespace re::literals;

		switch (semTypeName.Hashed())
		{ // clang-format off
		case "System.Int64"_hs: return "int64";
		case "System.Double"_hs: return "double";
		case "System.String"_hs: return "string";
		case "System.Boolean"_hs: return "bool";
		default: {
			std::cerr << "Unknown type '" << semTypeName << "'" << ". Fallback to System.Object" << std::endl;
			return "class [mscorlib]System.Object";
		}
		} // clang-format on
	}

	void DeclareLocal(const re::String& name, const ast::TypeNode* explicitType, const ast::Expr* initExpr)
	{
		re::String cilType = "int64"; // Fallback
		if (initExpr)
		{
			const auto semType = m_semanticAnalyzer.GetBindings().GetExpressionType(initExpr);
			if (semType)
			{
				cilType = MapTypeToCIL(semType->name);
			}
		}

		m_locals.push_back(name);
		m_localTypes.push_back(cilType);

		const int idx = static_cast<int>(m_locals.size() - 1);
		if (initExpr)
		{
			initExpr->Accept(*this);
		}
		else
		{
			*m_currentOut << "    ldc.i8 0\n"; // Fallback
		}

		*m_currentOut << "    stloc." << idx << " // " << name << "\n";
	}

	[[nodiscard]] int GetLocalIndex(const re::String& name) const
	{
		for (int i = 0; i < static_cast<int>(m_locals.size()); ++i)
		{
			if (m_locals[i] == name)
			{
				return i;
			}
		}
		return -1;
	}
};

} // namespace igni