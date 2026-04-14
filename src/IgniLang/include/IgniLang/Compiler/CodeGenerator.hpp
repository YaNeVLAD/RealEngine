#pragma once

#include <Core/HashedString.hpp>
#include <Core/String.hpp>
#include <IgniLang/AST/AstNodes.hpp>
#include <IgniLang/Semantic/SemanticAnalyzer.hpp>

#include <unordered_map>
#include <unordered_set>

namespace igni
{

// ==========================================
// PASS 2: Code Generator (Visitor)
// ==========================================
class CodeGenerator final : public ast::BaseAstVisitor
{
public:
	CodeGenerator(
		std::ostream& out,
		const std::vector<const ast::FunDecl*>& flatFuncs,
		const std::unordered_map<const ast::FunDecl*, std::vector<re::String>>& funcUpvals,
		const std::unordered_map<const ast::FunDecl*, std::unordered_set<re::String>>& funcBoxedVars,
		const std::unordered_map<re::String, re::String>& importAliases,
		const std::unordered_set<re::String>& externals,
		const sem::SemanticAnalyzer& semantics)
		: m_out(out)
		, m_flatFunctions(flatFuncs)
		, m_functionUpvalues(funcUpvals)
		, m_functionBoxedVars(funcBoxedVars)
		, m_importAliases(importAliases)
		, m_externals(externals)
		, m_semanticAnalyzer(semantics)
	{
	}

	void Generate(const ast::Program* program)
	{
		m_out << "// ==========================================\n";
		m_out << "// Auto-generated RVM Assembly\n";
		m_out << "// Package: " << program->packageName << "\n";
		m_out << "// ==========================================\n\n";

		m_funcAsmNames.clear();
		std::size_t funcId = 0;
		std::unordered_set<const ast::FunDecl*> globalFuncs;
		for (const auto& stmt : program->statements)
		{
			if (const auto fun = dynamic_cast<const ast::FunDecl*>(stmt.get()))
			{
				if (!fun->typeParams.empty())
				{
					continue;
				}

				globalFuncs.insert(fun);
			}
			else if (const auto classDecl = dynamic_cast<const ast::ClassDecl*>(stmt.get()))
			{
				if (!classDecl->typeParams.empty())
				{
					continue;
				}

				for (const auto& member : classDecl->members)
				{
					if (const auto memberFun = dynamic_cast<const ast::FunDecl*>(member.get()))
					{
						globalFuncs.insert(memberFun);
					}
				}
			}
			// --------------------------
		}

		for (const ast::FunDecl* fun : m_flatFunctions)
		{
			if (globalFuncs.contains(fun))
			{
				m_funcAsmNames[fun] = fun->name;
			}
			else
			{
				m_funcAsmNames[fun] = fun->name + "_fn_" + std::to_string(funcId++);
			}
		}

		m_out << "// --- Type Definitions ---\n";
		for (const auto& stmt : program->statements)
		{
			if (const auto classDecl = dynamic_cast<const ast::ClassDecl*>(stmt.get()))
			{
				if (classDecl->isExternal || !classDecl->typeParams.empty())
				{
					continue;
				}

				m_out << "TYPE \"" << classDecl->name << "\"\n";

				if (const auto classType = m_semanticAnalyzer.GetClassType(classDecl->name))
				{
					for (const auto& fieldName : classType->fields | std::views::keys)
					{
						m_out << "    FIELD \"" << fieldName << "\"\n";
					}
				}

				m_out << "END_TYPE\n\n";
			}
		}

		m_out << "// --- Global Initialization ---\n";
		m_currentFunction = nullptr;
		for (const auto& stmt : program->statements)
		{
			if (!dynamic_cast<const ast::FunDecl*>(stmt.get()))
			{
				stmt->Accept(*this);
			}
		}

		m_out << "// --- Entry Point ---\n";
		m_out << "CALL main 0\nRETURN\n\n";

		m_out << "// --- Function Definitions ---\n";
		for (const ast::FunDecl* fun : m_flatFunctions)
		{
			GenerateFunction(fun);
		}

		m_out << "// --- Constructor/Destructor Definitions ---\n";
		for (const auto& stmt : program->statements)
		{
			if (const auto classDecl = dynamic_cast<const ast::ClassDecl*>(stmt.get()))
			{
				if (classDecl->isExternal || !classDecl->typeParams.empty())
				{
					continue;
				}

				for (const auto& member : classDecl->members)
				{
					if (const auto ctor = dynamic_cast<const ast::ConstructorDecl*>(member.get()))
					{
						GenerateConstructor(ctor, classDecl);
					}
					else if (const auto dtor = dynamic_cast<const ast::DestructorDecl*>(member.get()))
					{
						GenerateDestructor(dtor, classDecl);
					}
				}
			}
		}
	}

	void Visit(const ast::MemberAccessExpr* node) override
	{
		if (node->object)
		{
			node->object->Accept(*this);
		}

		m_out << "GET_PROPERTY \"" << node->member << "\"\n";
	}

	void Visit(const ast::IndexExpr* node) override
	{
		if (node->array)
		{
			node->array->Accept(*this); // Push self
		}
		if (node->index)
		{
			node->index->Accept(*this); // Push arg1
		}
		m_out << "CALL_METHOD \"get\" 1\n";
	}

	void Visit(const ast::LiteralExpr* node) override
	{
		if (node->token.type == TokenType::KwTrue)
		{
			m_out << "CONST 1\n";
		}
		else if (node->token.type == TokenType::KwFalse)
		{
			m_out << "CONST 0\n";
		}
		else
		{
			m_out << "CONST " << node->token.lexeme << "\n";
		}
	}

	void Visit(const ast::IdentifierExpr* node) override
	{
		auto name = node->name;
		if (name.Hashed() == "super"_hs)
		{ // Aliasing super keyword
			name = "this";
		}

		if (m_importAliases.contains(name))
		{
			m_out << "GET_GLOBAL \"" << m_importAliases.at(name) << "\"\n";
			m_out << "GET_PROPERTY \"" << name << "\"\n";
			return;
		}

		if (const auto upIt = std::ranges::find(m_currentUpvalues, name); upIt != m_currentUpvalues.end())
		{
			m_out << "GET_UPVALUE " << std::distance(m_currentUpvalues.begin(), upIt) << "\n";
		}
		else if (IsLocal(name))
		{
			m_out << "GET " << GetAsmName(name) << "\n";
			if (m_functionBoxedVars.at(m_currentFunction).contains(name))
			{
				m_out << "UNBOX\n";
			}
		}
		else
		{
			const bool isGlobal = std::ranges::any_of(m_flatFunctions, [this, name](const ast::FunDecl* fun) {
				return fun->name == name && m_funcAsmNames.at(fun) == name;
			});
			isGlobal
				? m_out << "LOAD_FUN " << name << "\n"
				: m_out << "GET_GLOBAL \"" << name << "\"\n";
		}
	}

	void Visit(const ast::AssignExpr* node) override
	{
		if (const auto id = dynamic_cast<const ast::IdentifierExpr*>(node->target.get()))
		{
			const auto& name = id->name;
			if (const auto upIt = std::ranges::find(m_currentUpvalues, name); upIt != m_currentUpvalues.end())
			{
				if (node->value)
				{
					node->value->Accept(*this);
				}
				m_out << "SET_UPVALUE " << std::distance(m_currentUpvalues.begin(), upIt) << "\n";
			}
			else if (IsLocal(name))
			{
				const re::String asmName = GetAsmName(name);
				if (m_currentFunction && m_functionBoxedVars.at(m_currentFunction).contains(name))
				{
					m_out << "GET " << asmName << "\n";
					if (node->value)
					{
						node->value->Accept(*this);
					}
					m_out << "STORE_BOX\n";
				}
				else
				{
					if (node->value)
					{
						node->value->Accept(*this);
					}
					m_out << "SET " << asmName << "\n";
				}
			}
			else
			{
				if (node->value)
				{
					node->value->Accept(*this);
				}
				m_out << "SET_GLOBAL \"" << name << "\"\n";
			}
			m_out << "CONST 0\n";
		}
		else if (const auto idx = dynamic_cast<const ast::IndexExpr*>(node->target.get()))
		{
			if (idx->array)
			{
				idx->array->Accept(*this); // self
			}
			if (idx->index)
			{
				idx->index->Accept(*this); // arg1
			}
			if (node->value)
			{
				node->value->Accept(*this); // arg2
			}
			m_out << "CALL_METHOD \"set\" 2\n";
		}
		else if (const auto memAccess = dynamic_cast<const ast::MemberAccessExpr*>(node->target.get()))
		{
			if (memAccess->object)
			{
				memAccess->object->Accept(*this);
			}
			if (node->value)
			{
				node->value->Accept(*this);
			}
			m_out << "SET_PROPERTY \"" << memAccess->member << "\"\n";
			m_out << "CONST 0\n";
		}
	}

	void Visit(const ast::CallExpr* node) override
	{
		if (node->isSuperCall)
		{
			const auto thisExpr = std::make_unique<ast::IdentifierExpr>();
			thisExpr->name = "this";
			thisExpr->Accept(*this);

			for (const auto& arg : node->arguments)
			{
				arg->Accept(*this);
			}

			m_out << "CALL " << node->staticMethodTarget << " " << (node->arguments.size() + 1) << "\n";
			return;
		}
		else if (node->isConstructorCall)
		{
			re::String className;

			if (const auto id = dynamic_cast<const ast::IdentifierExpr*>(node->callee.get()))
			{
				className = id->name;
			}
			else if (const auto memAccess = dynamic_cast<const ast::MemberAccessExpr*>(node->callee.get()))
			{
				className = memAccess->member;
			}

			m_out << "NEW \"" << className << "\"\n";

			m_out << "DUP\n";

			for (const auto& arg : node->arguments)
			{
				if (arg)
				{
					arg->Accept(*this);
				}
			}

			if (node->isVarargCall)
			{
				m_out << "PACK_ARRAY " << node->varargCount << "\n";
			}
			const std::size_t actualArgs = node->isVarargCall
				? (node->arguments.size() - node->varargCount + 1)
				: node->arguments.size();

			if (!node->staticMethodTarget.Empty())
			{
				m_out << "CALL " << node->staticMethodTarget << " " << (actualArgs + 1) << "\n";
			}
			else if (actualArgs > 0)
			{
				m_out << "CALL_METHOD \"init\" " << actualArgs << "\n";
			}

			m_out << "POP\n";

			return;
		}

		if (!node->staticMethodTarget.Empty())
		{
			if (const auto memberAccess = dynamic_cast<const ast::MemberAccessExpr*>(node->callee.get()); memberAccess)
			{
				if (memberAccess->object)
				{
					memberAccess->object->Accept(*this);
				}
			}

			// 2. Кладем остальные аргументы
			for (const auto& arg : node->arguments)
			{
				if (arg)
				{
					arg->Accept(*this);
				}
			}

			if (node->isVarargCall)
			{
				m_out << "PACK_ARRAY " << node->varargCount << "\n";
			}

			const std::size_t actualArgs = node->isVarargCall
				? (node->arguments.size() - node->varargCount + 1)
				: node->arguments.size();

			// Call function with one extra argument (this)
			m_out << "CALL " << node->staticMethodTarget << " " << (actualArgs + 1) << "\n";
			return;
		}

		if (const auto memberAccess = dynamic_cast<const ast::MemberAccessExpr*>(node->callee.get()))
		{
			if (memberAccess->object)
			{
				memberAccess->object->Accept(*this);
			}

			for (const auto& arg : node->arguments)
			{
				if (arg)
				{
					arg->Accept(*this);
				}
			}

			if (node->isVarargCall)
			{
				m_out << "PACK_ARRAY " << node->varargCount << "\n";
			}
			const std::size_t actualArgs = node->isVarargCall
				? (node->arguments.size() - node->varargCount + 1)
				: node->arguments.size();

			m_out << "CALL_METHOD \"" << memberAccess->member << "\" " << actualArgs << "\n";
			return;
		}

		bool isDirectCall = false;
		re::String directFuncName;
		if (const auto id = dynamic_cast<const ast::IdentifierExpr*>(node->callee.get()))
		{
			directFuncName = id->name;
			if (m_importAliases.contains(directFuncName))
			{
				const auto& moduleName = m_importAliases.at(directFuncName);
				m_out << "GET_GLOBAL \"" << moduleName << "\"\n";

				for (const auto& arg : node->arguments)
				{
					if (arg)
					{
						arg->Accept(*this);
					}
				}

				if (node->isVarargCall)
				{
					m_out << "PACK_ARRAY " << node->varargCount << "\n";
				}
				const std::size_t actualArgs = node->isVarargCall
					? (node->arguments.size() - node->varargCount + 1)
					: node->arguments.size();

				m_out << "CALL_METHOD \"" << directFuncName << "\" " << actualArgs << "\n";
				return;
			}

			if (m_externals.contains(directFuncName))
			{
				isDirectCall = true;
			}
			else if (!IsLocal(directFuncName)
				&& std::ranges::find(m_currentUpvalues, directFuncName) == m_currentUpvalues.end())
			{
				isDirectCall = true;
			}
		}

		if (isDirectCall)
		{
			for (const auto& arg : node->arguments)
			{
				if (arg)
				{
					arg->Accept(*this);
				}
			}

			if (node->isVarargCall)
			{
				m_out << "PACK_ARRAY " << node->varargCount << "\n";
			}
			const std::size_t actualArgs = node->isVarargCall
				? (node->arguments.size() - node->varargCount + 1)
				: node->arguments.size();

			if (m_externals.contains(directFuncName))
			{
				m_out << "NATIVE \"" << directFuncName << "\" " << actualArgs << "\n";
			}
			else
			{
				m_out << "CALL " << directFuncName << " " << actualArgs << "\n";
			}
		}
		else
		{
			if (node->callee)
			{
				node->callee->Accept(*this);
			}

			for (const auto& arg : node->arguments)
			{
				if (arg)
				{
					arg->Accept(*this);
				}
			}

			if (node->isVarargCall)
			{
				m_out << "PACK_ARRAY " << node->varargCount << "\n";
			}

			const std::size_t actualArgs = node->isVarargCall
				? (node->arguments.size() - node->varargCount + 1)
				: node->arguments.size();

			m_out << "CALL_INDIRECT " << actualArgs << "\n";
		}
	}

	void Visit(const ast::UnaryExpr* node) override
	{
		const auto& op = node->op;
		if (const auto hashed = op.Hashed(); hashed == "-"_hs)
		{
			m_out << "CONST 0\n";
			if (node->operand)
			{
				node->operand->Accept(*this);
			}
			m_out << "SUB\n";
		}
		else if (hashed == "+"_hs)
		{
			if (node->operand)
			{
				node->operand->Accept(*this);
			}
		}
		else if (hashed == "!"_hs)
		{
			if (node->operand)
			{
				node->operand->Accept(*this);
			}
			m_out << "CONST 0\nEQUAL\n";
		}
		else if (hashed == "~"_hs)
		{
			if (node->operand)
			{
				node->operand->Accept(*this);
			}
			m_out << "BIT_NOT\n";
		}
		else if (hashed == "++"_hs || hashed == "--"_hs)
		{
			const auto vmOp = (hashed == "++"_hs) ? "INC" : "DEC";
			const auto revOp = (hashed == "++"_hs) ? "DEC" : "INC";

			if (const auto id = dynamic_cast<const ast::IdentifierExpr*>(node->operand.get()))
			{
				const auto& name = id->name;
				const auto upIt = std::ranges::find(m_currentUpvalues, name);

				if (upIt != m_currentUpvalues.end())
				{
					m_out << "GET_UPVALUE " << std::distance(m_currentUpvalues.begin(), upIt) << "\n";
					m_out << vmOp << "\n";
					m_out << "SET_UPVALUE " << std::distance(m_currentUpvalues.begin(), upIt) << "\n";
				}
				else
				{
					const re::String asmName = IsLocal(name) ? GetAsmName(name) : name;

					if (m_functionBoxedVars.at(m_currentFunction).contains(name))
					{
						m_out << "GET " << asmName << "\nGET " << asmName << "\nUNBOX\n"
							  << vmOp << "\nSTORE_BOX\n";
					}
					else
					{
						m_out << "GET " << asmName << "\n"
							  << vmOp << "\nSET " << asmName << "\n";
					}
				}

				if (upIt != m_currentUpvalues.end())
				{
					m_out << "GET_UPVALUE " << std::distance(m_currentUpvalues.begin(), upIt) << "\n";
				}
				else
				{
					const re::String asmName = IsLocal(name) ? GetAsmName(name) : name;
					m_out << "GET " << asmName << "\n";
					if (m_functionBoxedVars.at(m_currentFunction).contains(name))
					{
						m_out << "UNBOX\n";
					}
				}

				if (node->isPostfix)
				{
					m_out << revOp << "\n";
				}
			}
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

		// clang-format off
		switch (node->op.Hash())
		{
		case "+"_hs:  m_out << "ADD\n"; break;
		case "-"_hs:  m_out << "SUB\n"; break;
		case "*"_hs:  m_out << "MUL\n"; break;
		case "/"_hs:  m_out << "DIV\n"; break;
		case "%"_hs:  m_out << "MOD\n"; break;
		case "<"_hs:  m_out << "LESS\n"; break;
		case ">"_hs:  m_out << "GREATER\n"; break;
		case "<="_hs: m_out << "LESS_EQUAL\n"; break;
		case ">="_hs: m_out << "GREATER_EQUAL\n"; break;
		case "=="_hs: m_out << "EQUAL\n"; break;
		case "!="_hs: m_out << "NOT_EQUAL\n"; break;
		case "&&"_hs: m_out << "AND\n"; break;
		case "||"_hs: m_out << "OR\n"; break;
		default: throw std::runtime_error("TextCompiler: Unsupported binary operator '" + node->op.ToString() + "'");
		}
		// clang-format on
	}

	void Visit(const ast::ExprStmt* node) override
	{
		if (node->expr)
		{
			node->expr->Accept(*this);
		}

		bool shouldPop = true;
		if (const auto call = dynamic_cast<const ast::CallExpr*>(node->expr.get()))
		{
			if (const auto id = dynamic_cast<const ast::IdentifierExpr*>(call->callee.get()))
			{
				if (m_externals.contains(id->name))
				{
					shouldPop = false;
				}
			}
		}
		if (shouldPop)
		{
			m_out << "POP\n";
		}
	}

	void Visit(const ast::ReturnStmt* node) override
	{
		if (node->expr)
		{
			node->expr->Accept(*this);
		}
		else
		{
			m_out << "CONST 0\n";
		}
		m_out << "RETURN\n";
	}

	void Visit(const ast::Block* node) override
	{
		const std::size_t localsCountBefore = m_currentLocals.size();

		for (const auto& s : node->statements)
		{
			if (s)
			{
				s->Accept(*this);
			}
		}

		m_currentLocals.resize(localsCountBefore);
	}

	void Visit(const ast::IfStmt* node) override
	{
		const auto currentLabelId = m_labelCount++;
		const re::String elseLabel = "L_else_" + std::to_string(currentLabelId);
		const re::String endLabel = "L_end_" + std::to_string(currentLabelId);

		m_out << "// --- if ---\n";
		if (node->condition)
		{
			node->condition->Accept(*this);
		}

		m_out << "JMP_IF_FALSE " << (node->elseBranch ? elseLabel : endLabel) << "\n";

		if (node->thenBranch)
		{
			node->thenBranch->Accept(*this);
		}

		if (node->elseBranch)
		{
			m_out << "JMP " << endLabel << "\nLABEL " << elseLabel << "\n";
			node->elseBranch->Accept(*this);
		}
		m_out << "LABEL " << endLabel << "\n";
	}

	void Visit(const ast::WhileStmt* node) override
	{
		const auto labelId = m_labelCount++;
		const re::String startLabel = "L_while_start_" + std::to_string(labelId);
		const re::String endLabel = "L_while_end_" + std::to_string(labelId);

		m_out << "LABEL " << startLabel << "\n";
		if (node->condition)
		{
			node->condition->Accept(*this);
		}
		m_out << "JMP_IF_FALSE " << endLabel << "\n";

		if (node->body)
		{
			node->body->Accept(*this);
		}

		m_out << "JMP " << startLabel << "\nLABEL " << endLabel << "\n";
	}

	void Visit(const ast::ForStmt* node) override
	{
		const auto labelId = m_labelCount++;
		const re::String startLabel = "L_for_start_" + std::to_string(labelId);
		const re::String endLabel = "L_for_end_" + std::to_string(labelId);

		const std::size_t localsCountBefore = m_currentLocals.size();

		if (node->isForEach)
		{
			m_out << "// --- for-each (" << node->iteratorName << ") ---\n";

			node->startExpr->Accept(*this);
			const auto asmArrName = DeclareLocal("__range" + std::to_string(labelId));
			m_out << "SET " << asmArrName << "\n";

			m_out << "GET " << asmArrName << "\n";
			m_out << "GET_PROPERTY \"size\"\n";
			const auto asmSizeName = DeclareLocal("__size" + std::to_string(labelId));
			m_out << "SET " << asmSizeName << "\n";

			m_out << "CONST 0\n";
			const auto asmIndexName = DeclareLocal("__it" + std::to_string(labelId));
			m_out << "SET " << asmIndexName << "\n";

			m_out << "LABEL " << startLabel << "\n";

			m_out << "GET " << asmIndexName << "\n";
			m_out << "GET " << asmSizeName << "\n";
			m_out << "LESS\n";
			m_out << "JMP_IF_FALSE " << endLabel << "\n";

			m_out << "GET " << asmArrName << "\n";
			m_out << "GET " << asmIndexName << "\n";
			m_out << "CALL_METHOD \"get\" 1\n";
			const auto asmIterName = DeclareLocal(node->iteratorName);
			m_out << "SET " << asmIterName << "\n";

			if (node->body)
			{
				node->body->Accept(*this);
			}

			m_out << "GET " << asmIndexName << "\n";
			m_out << "INC\n";
			m_out << "SET " << asmIndexName << "\n";

			m_out << "JMP " << startLabel << "\n";
			m_out << "LABEL " << endLabel << "\n";
		}
		else
		{
			const auto& iterName = node->iteratorName;
			const re::String limitName = "_for_limit_" + std::to_string(labelId);

			m_out << "// --- for (" << iterName << ") ---\n";

			if (node->startExpr)
			{
				node->startExpr->Accept(*this);
			}
			const auto asmIterName = DeclareLocal(iterName);
			m_out << "SET " << asmIterName << "\n";

			if (node->endExpr)
			{
				node->endExpr->Accept(*this);
			}
			const auto asmLimitName = DeclareLocal(limitName);
			m_out << "SET " << asmLimitName << "\n";

			m_out << "LABEL " << startLabel << "\n";
			m_out << "CONST 1\nGET " << asmLimitName << "\nGET " << asmIterName << "\nLESS\nSUB\n";
			m_out << "JMP_IF_FALSE " << endLabel << "\n";

			if (node->body)
			{
				node->body->Accept(*this);
			}

			m_out << "GET " << asmIterName << "\nINC\nSET " << asmIterName << "\n";
			m_out << "JMP " << startLabel << "\nLABEL " << endLabel << "\n";
		}

		m_currentLocals.resize(localsCountBefore);
	}

	void Visit(const ast::VarDecl* node) override
	{
		if (node->initializer)
		{
			node->initializer->Accept(*this);
		}
		else
		{
			m_out << "CONST 0\n";
		}

		if (m_currentFunction == nullptr)
		{ // Global var declaration
			m_out << "SET_GLOBAL \"" << node->name << "\"\n";
		}
		else
		{
			if (m_functionBoxedVars.at(m_currentFunction).contains(node->name))
			{
				m_out << "BOX\n";
			}
			const re::String asmName = DeclareLocal(node->name);
			m_out << "SET " << asmName << "\n";
		}
	}

	void Visit(const ast::ValDecl* node) override
	{
		if (node->initializer)
		{
			node->initializer->Accept(*this);
		}
		else
		{
			m_out << "CONST 0\n";
		}

		if (m_currentFunction == nullptr)
		{ // Global val declaration
			m_out << "SET_GLOBAL \"" << node->name << "\"\n";
		}
		else
		{
			if (m_functionBoxedVars.at(m_currentFunction).contains(node->name))
			{
				m_out << "BOX\n";
			}
			const re::String asmName = DeclareLocal(node->name);
			m_out << "SET " << asmName << "\n";
		}
	}

	void Visit(const ast::FunDecl* node) override
	{
		if (node->isExternal)
		{
			return;
		}

		const auto& upvalues = m_functionUpvalues.at(node);
		for (const auto& uv : upvalues)
		{
			if (auto upIt = std::ranges::find(m_currentUpvalues, uv); upIt != m_currentUpvalues.end())
			{
				m_out << "GET_UPVALUE " << std::distance(m_currentUpvalues.begin(), upIt) << "\n";
			}
			else
			{
				m_out << "GET " << GetAsmName(uv) << "\n";
			}
		}

		m_out << "MAKE_CLOSURE " << m_funcAsmNames.at(node) << " " << upvalues.size() << "\n";
		m_out << "SET " << DeclareLocal(node->name) << "\n";
	}

private:
	std::ostream& m_out;
	std::size_t m_labelCount = 0;
	std::size_t m_varCounter = 0;

	const ast::FunDecl* m_currentFunction = nullptr;
	std::vector<std::pair<re::String, re::String>> m_currentLocals;
	std::vector<re::String> m_currentUpvalues;

	const std::vector<const ast::FunDecl*>& m_flatFunctions;
	const std::unordered_map<const ast::FunDecl*, std::vector<re::String>>& m_functionUpvalues;
	const std::unordered_map<const ast::FunDecl*, std::unordered_set<re::String>>& m_functionBoxedVars;
	std::unordered_map<const ast::FunDecl*, re::String> m_funcAsmNames;

	const std::unordered_map<re::String, re::String>& m_importAliases;
	const std::unordered_set<re::String>& m_externals;

	const sem::SemanticAnalyzer& m_semanticAnalyzer;

	void GenerateConstructor(const ast::ConstructorDecl* ctor, const ast::ClassDecl* classDecl)
	{
		m_out << "FUN " << ctor->name << "\n";
		m_currentLocals.clear();
		m_varCounter = 0;

		m_currentUpvalues.clear(); // TODO: Think about upvalues in constructors

		for (auto it = ctor->parameters.rbegin(); it != ctor->parameters.rend(); ++it)
		{
			m_out << "SET " << DeclareLocal(it->name) << "\n";
		}

		// Injecting member initializers
		const re::String thisAsmName = GetAsmName("this");
		for (const auto& member : classDecl->members)
		{
			if (const auto varDecl = dynamic_cast<const ast::VarDecl*>(member.get()))
			{
				if (varDecl->initializer)
				{
					m_out << "GET " << thisAsmName << "\n";
					varDecl->initializer->Accept(*this);
					m_out << "SET_PROPERTY \"" << varDecl->name << "\"\n";
				}
			}
			else if (const auto valDecl = dynamic_cast<const ast::ValDecl*>(member.get()))
			{
				if (valDecl->initializer)
				{
					m_out << "GET " << thisAsmName << "\n";
					valDecl->initializer->Accept(*this);
					m_out << "SET_PROPERTY \"" << valDecl->name << "\"\n";
				}
			}
		}

		if (ctor->body)
		{
			for (const auto& s : ctor->body->statements)
			{
				if (s)
				{
					s->Accept(*this);
				}
			}
		}

		m_out << "CONST 0\nRETURN\n\n";
	}

	void GenerateDestructor(const ast::DestructorDecl* dtor, const ast::ClassDecl* classDecl)
	{
		m_out << "FUN " << dtor->name << "\n";
		m_currentLocals.clear();
		m_varCounter = 0;
		m_currentUpvalues.clear();

		m_out << "SET " << DeclareLocal("this") << "\n";

		if (dtor->body)
		{
			for (const auto& s : dtor->body->statements)
			{
				if (s)
				{
					s->Accept(*this);
				}
			}
		}

		m_out << "CONST 0\nRETURN\n\n";
	}

	re::String DeclareLocal(const re::String& originalName)
	{
		re::String asmName = originalName + "_" + std::to_string(m_varCounter++);
		m_currentLocals.emplace_back(originalName, asmName);
		return asmName;
	}

	re::String GetAsmName(const re::String& name) const
	{
		for (auto it = m_currentLocals.rbegin(); it != m_currentLocals.rend(); ++it)
		{
			if (it->first == name)
			{
				return it->second;
			}
		}

		return name;
	}

	bool IsLocal(const re::String& name) const
	{
		for (const auto& key : m_currentLocals | std::views::keys)
		{
			if (key == name)
			{
				return true;
			}
		}

		return false;
	}

	void GenerateFunction(const ast::FunDecl* fun)
	{
		if (!fun->typeParams.empty())
		{
			return;
		}

		m_currentFunction = fun;
		m_out << "FUN " << m_funcAsmNames.at(fun) << "\n";
		m_currentLocals.clear();
		m_varCounter = 0;
		m_currentUpvalues = m_functionUpvalues.at(fun);

		const auto& boxedVars = m_functionBoxedVars.at(fun);

		for (auto it = fun->parameters.rbegin(); it != fun->parameters.rend(); ++it)
		{
			auto paramName = it->name;
			if (boxedVars.contains(paramName))
			{
				m_out << "BOX\n";
			}

			m_out << "SET " << DeclareLocal(paramName) << "\n";
		}

		if (fun->body)
		{
			for (const auto& s : fun->body->statements)
			{ // Avoiding recursive code generation by manually calling Accept for function statements
				if (s)
				{
					s->Accept(*this);
				}
			}
		}

		m_out << "CONST 0\nRETURN\n\n";
	}
};

} // namespace igni