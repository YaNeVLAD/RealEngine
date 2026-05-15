#pragma once

#include <IgniLang/AST/AstNodes.hpp>
#include <IgniLang/Semantic/SemanticAnalyzer.hpp>

#include <ostream>
#include <sstream>
#include <vector>

namespace igni
{

class DotNetCodeGenerator final : public ast::BaseAstVisitor
{
public:
	static constexpr auto TYPE_OBJECT = "class [mscorlib]System.Object";
	static constexpr auto TYPE_VOID = "void";

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
		m_currentClass = nullptr;

		for (const auto& stmt : program->statements)
		{
			if (!dynamic_cast<const ast::ClassDecl*>(stmt.get()) && stmt)
			{
				stmt->Accept(*this);
			}
		}
		m_out << "}\n\n";

		for (const auto& stmt : program->statements)
		{
			if (const auto classDecl = dynamic_cast<const ast::ClassDecl*>(stmt.get()))
			{
				classDecl->Accept(*this);
			}
		}
	}

	void Visit(const ast::ClassDecl* node) override
	{
		if (node->isExternal || !node->typeParams.empty())
			return;

		m_currentClass = node;
		m_out << ".class public auto ansi beforefieldinit " << node->name << " extends [mscorlib]System.Object\n{\n";

		if (const auto semClass = m_semanticAnalyzer.GetClassType(node->name))
		{
			for (const auto& [fieldName, fieldInfo] : semClass->fields)
			{
				m_out << "  .field public " << MapTypeToCIL(fieldInfo.type->name) << " '" << fieldName << "'\n";
			}
		}

		for (const auto& member : node->members)
		{
			if (dynamic_cast<const ast::FunDecl*>(member.get()) || dynamic_cast<const ast::ConstructorDecl*>(member.get()))
			{
				member->Accept(*this);
			}
		}

		m_out << "}\n\n";
		m_currentClass = nullptr;
	}

	void Visit(const ast::ConstructorDecl* node) override
	{
		if (node->isExternal)
		{
			return;
		}

		PrepareMethodScope(true, node->parameters);

		*m_currentOut << "    ldarg.0\n    call instance void [mscorlib]System.Object::.ctor()\n";

		const re::String sig = ".method public hidebysig specialname rtspecialname instance void .ctor(" + BuildParamSignature(node->parameters) + ") cil managed";
		EmitMethodBody(sig, node->body.get(), false);
	}

	void Visit(const ast::FunDecl* node) override
	{
		if (node->isExternal)
		{
			return;
		}

		PrepareMethodScope(m_currentClass != nullptr, node->parameters);

		const re::String retType = node->returnType ? MapAstType(node->returnType.get()) : TYPE_VOID;
		const re::String instanceKw = m_currentClass ? "instance " : "static ";
		const bool isEntryPoint = (node->name == "main" && !m_currentClass);

		const re::String sig = ".method public hidebysig " + instanceKw + retType + " " + node->name + "(" + BuildParamSignature(node->parameters) + ") cil managed";
		EmitMethodBody(sig, node->body.get(), isEntryPoint);
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
		DeclareLocal(node->name, node->initializer.get());
	}

	void Visit(const ast::ValDecl* node) override
	{
		DeclareLocal(node->name, node->initializer.get());
	}

	void Visit(const ast::AssignExpr* node) override
	{
		if (const auto memAccess = dynamic_cast<const ast::MemberAccessExpr*>(node->target.get()))
		{
			memAccess->object->Accept(*this);
			if (node->value)
			{
				node->value->Accept(*this);
			}
			EmitFieldAccess(memAccess->object.get(), memAccess->member, true);
		}
		else if (const auto id = dynamic_cast<const ast::IdentifierExpr*>(node->target.get()))
		{
			if (!m_semanticAnalyzer.GetBindings().implicitThisNames.contains(id) && node->value)
			{
				node->value->Accept(*this);
			}
			EmitIdentifierAccess(id, true, node->value.get());
		}
	}

	void Visit(const ast::MemberAccessExpr* node) override
	{
		if (node->object)
		{
			node->object->Accept(*this);
		}
		EmitFieldAccess(node->object.get(), node->member, false);
	}

	void Visit(const ast::IdentifierExpr* node) override
	{
		EmitIdentifierAccess(node, false, nullptr);
	}

	void Visit(const ast::LiteralExpr* node) override
	{
		switch (node->token.type)
		{ // clang-format off
		case TokenType::IntConst:    *m_currentOut << "    ldc.i8 " << node->token.lexeme << "\n"; break;
		case TokenType::FloatConst:  *m_currentOut << "    ldc.r8 " << node->token.lexeme << "\n"; break;
		case TokenType::StringConst: *m_currentOut << "    ldstr " << node->token.lexeme << "\n"; break;
		case TokenType::KwTrue:      *m_currentOut << "    ldc.i4.1\n"; break;
		case TokenType::KwFalse:     *m_currentOut << "    ldc.i4.0\n"; break;
		default: break;
        } // clang-format on
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
		const std::string elseLabel = "L_else_" + std::to_string(m_labelCount);
		const std::string endLabel = "L_end_" + std::to_string(m_labelCount++);

		if (node->condition)
		{
			node->condition->Accept(*this);
		}

		*m_currentOut << "    brfalse " << (node->elseBranch ? elseLabel : endLabel) << "\n";

		if (node->thenBranch)
		{
			node->thenBranch->Accept(*this);
		}

		if (node->elseBranch)
		{
			*m_currentOut << "    br " << endLabel << "\n"
						  << elseLabel << ":\n";
			node->elseBranch->Accept(*this);
		}

		*m_currentOut << endLabel << ":\n";
	}

	void Visit(const ast::WhileStmt* node) override
	{
		const std::string startLabel = "L_while_start_" + std::to_string(m_labelCount);
		const std::string endLabel = "L_while_end_" + std::to_string(m_labelCount++);

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
		*m_currentOut << "    br " << startLabel << "\n"
					  << endLabel << ":\n";
	}

	void Visit(const ast::ForStmt* node) override
	{
		if (node->isForEach)
		{
			return; // TODO: Add Array iteration
		}

		const std::string startLabel = "L_for_start_" + std::to_string(m_labelCount);
		const std::string endLabel = "L_for_end_" + std::to_string(m_labelCount++);

		DeclareLocal(node->iteratorName, node->startExpr.get());
		const int iterIdx = GetLocalIndex(node->iteratorName);

		const std::string limitName = "_for_limit_" + std::to_string(m_labelCount);
		DeclareLocal(limitName, node->endExpr.get());
		const int limitIdx = GetLocalIndex(limitName);

		*m_currentOut << startLabel << ":\n"
					  << "    ldloc." << iterIdx << "\n"
					  << "    ldloc." << limitIdx << "\n"
					  << "    bgt " << endLabel << "\n";

		if (node->body)
		{
			node->body->Accept(*this);
		}

		*m_currentOut << "    ldloc." << iterIdx << "\n    ldc.i8 1\n    add\n    stloc." << iterIdx << "\n"
					  << "    br " << startLabel << "\n"
					  << endLabel << ":\n";
	}

	void Visit(const ast::CallExpr* node) override
	{
		const auto& callInfo = m_semanticAnalyzer.GetBindings().callInfo.at(node);

		auto emitArgs = [&] {
			for (const auto& arg : node->arguments)
			{
				if (arg)
				{
					arg->Accept(*this);
				}
			}
		};

		if (callInfo.dispatchMode == CallDispatchType::Native)
		{
			emitArgs();
			*m_currentOut << "    call " << callInfo.asmLabel << "\n";
			return;
		}

		if (callInfo.isConstructorCall)
		{
			emitArgs();
			*m_currentOut << "    newobj instance void " << callInfo.mangledClassName << "::.ctor(" << BuildTypeSignature(callInfo.target->paramTypes, 1) << ")\n";
			return;
		}

		if (callInfo.dispatchMode == CallDispatchType::Virtual)
		{
			if (const auto memAccess = dynamic_cast<const ast::MemberAccessExpr*>(node->callee.get()))
			{
				memAccess->object->Accept(*this);
			}
			else if (callInfo.isImplicitThisCall)
			{
				*m_currentOut << "    ldarg.0 // implicit this\n";
			}

			emitArgs();
			*m_currentOut << "    callvirt instance " << MapTypeToCIL(callInfo.target->returnType->name)
						  << " " << callInfo.target->paramTypes[0]->name << "::" << callInfo.asmLabel
						  << "(" << BuildTypeSignature(callInfo.target->paramTypes, 1) << ")\n";

			return;
		}

		emitArgs();
		const re::String retType = callInfo.target && callInfo.target->returnType
			? MapTypeToCIL(callInfo.target->returnType->name)
			: TYPE_VOID;
		*m_currentOut << "    call " << retType << " Program::" << callInfo.asmLabel << "()\n";
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
	std::stringstream m_methodBuffer;
	const sem::SemanticAnalyzer& m_semanticAnalyzer;

	const ast::ClassDecl* m_currentClass = nullptr;
	std::vector<re::String> m_locals;
	std::vector<re::String> m_localTypes;
	std::vector<re::String> m_args;
	std::size_t m_labelCount = 0;

	void PrepareMethodScope(const bool hasThis, const std::vector<ast::Parameter>& params)
	{
		m_locals.clear();
		m_localTypes.clear();
		m_args.clear();
		if (hasThis)
		{
			m_args.emplace_back("this");
		}
		for (const auto& [name, _] : params)
		{
			m_args.push_back(name);
		}

		m_methodBuffer.str("");
		m_methodBuffer.clear();
		m_currentOut = &m_methodBuffer;
	}

	void EmitMethodBody(const re::String& signature, const ast::Block* body, const bool isEntryPoint)
	{
		if (body)
		{
			body->Accept(*this);
		}
		m_currentOut = &m_out;

		m_out << "  " << signature << "\n  {\n";
		if (isEntryPoint)
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

		m_out << m_methodBuffer.str() << "    ret\n  }\n\n";
	}

	std::shared_ptr<sem::ClassType> ResolveClassType(const ast::Expr* objExpr) const
	{
		const auto objSemType = m_semanticAnalyzer.GetBindings().GetExpressionType(objExpr);
		if (!objSemType)
		{
			if (const auto id = dynamic_cast<const ast::IdentifierExpr*>(objExpr); id && id->name == "this")
			{
				return m_semanticAnalyzer.GetClassType(m_currentClass->name);
			}
		}

		return std::dynamic_pointer_cast<sem::ClassType>(objSemType);
	}

	void EmitFieldAccess(const ast::Expr* objNode, const re::String& member, const bool isStore) const
	{
		if (const auto classType = ResolveClassType(objNode))
		{
			if (classType->fields.contains(member))
			{
				const auto fieldType = classType->fields.at(member).type;
				*m_currentOut << "    " << (isStore ? "stfld " : "ldfld ") << MapTypeToCIL(fieldType->name)
							  << " " << classType->name << "::" << member << "\n";
			}
		}
	}

	void EmitIdentifierAccess(const ast::IdentifierExpr* id, const bool isStore, const ast::Expr* assignValue)
	{
		if (m_semanticAnalyzer.GetBindings().implicitThisNames.contains(id))
		{
			*m_currentOut << "    ldarg.0 // this\n";
			if (assignValue)
			{
				assignValue->Accept(*this);
			}

			const auto classType = m_semanticAnalyzer.GetClassType(m_currentClass->name);
			const auto fieldType = classType->fields.at(id->name).type;
			*m_currentOut << "    " << (isStore ? "stfld " : "ldfld ") << MapTypeToCIL(fieldType->name)
						  << " " << classType->name << "::" << id->name << "\n";

			return;
		}

		if (const int locIdx = GetLocalIndex(id->name); locIdx != -1)
		{
			*m_currentOut << "    " << (isStore ? "stloc." : "ldloc.") << locIdx << " // " << id->name << "\n";
			return;
		}
		if (const int argIdx = GetArgIndex(id->name); argIdx != -1)
		{
			*m_currentOut << "    " << (isStore ? "starg." : "ldarg.") << argIdx << " // " << id->name << "\n";
		}
	}

	void DeclareLocal(const re::String& name, const ast::Expr* initExpr)
	{
		re::String cilType = "int64"; // Fallback
		if (initExpr)
		{
			if (const auto semType = m_semanticAnalyzer.GetBindings().GetExpressionType(initExpr))
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
			*m_currentOut << "    ldc.i8 0\n";
		}

		*m_currentOut << "    stloc." << idx << " // " << name << "\n";
	}

	static re::String MapTypeToCIL(const re::String& semTypeName)
	{
		using namespace re::literals;
		switch (semTypeName.Hashed())
		{ // clang-format off
        case "System.Int64"_hs:   return "int64";
        case "System.Double"_hs:  return "float64";
        case "System.String"_hs:  return "string";
        case "System.Boolean"_hs: return "bool";
        case "Unit"_hs:
		case "System.Void"_hs:    return TYPE_VOID;
        case "Any"_hs:
		case "System.Object"_hs:  return TYPE_OBJECT;
        default:                  return "class " + semTypeName;
       } // clang-format on
	}

	static re::String MapAstType(const ast::TypeNode* node)
	{
		using namespace re::literals;

		if (const auto s = dynamic_cast<const ast::SimpleTypeNode*>(node))
		{
			switch (s->name.Hashed())
			{ // clang-format off
			case "Int"_hs:    return "int64";
			case "Double"_hs: return "float64";
			case "String"_hs: return "string";
			case "Bool"_hs:   return "bool";
			case "Unit"_hs:   return TYPE_VOID;
			case "Any"_hs:    return TYPE_OBJECT;
			default:          return "class " + s->name;
			} // clang-format on
		}

		return TYPE_OBJECT;
	}

	[[nodiscard]] static std::string BuildParamSignature(const std::vector<ast::Parameter>& params)
	{
		std::string sig;
		for (std::size_t i = 0; i < params.size(); ++i)
		{
			sig += MapAstType(params[i].type.get()).ToString();
			if (i < params.size() - 1)
			{
				sig += ", ";
			}
		}

		return sig;
	}

	[[nodiscard]] static std::string BuildTypeSignature(const std::vector<std::shared_ptr<sem::SemanticType>>& types, const std::size_t startIndex)
	{
		std::string sig;
		for (std::size_t i = startIndex; i < types.size(); ++i)
		{
			sig += MapTypeToCIL(types[i]->name).ToString();
			if (i < types.size() - 1)
			{
				sig += ", ";
			}
		}

		return sig;
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

	[[nodiscard]] int GetArgIndex(const re::String& name) const
	{
		for (int i = 0; i < static_cast<int>(m_args.size()); ++i)
		{
			if (m_args[i] == name)
			{
				return i;
			}
		}

		return -1;
	}
};

} // namespace igni