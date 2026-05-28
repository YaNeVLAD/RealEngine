#pragma once

#include <Core/flat_map.hpp>
#include <IgniLang/AST/AstNodes.hpp>
#include <IgniLang/AST/Utils/AnnotationUtils.hpp>
#include <IgniLang/Compiler/DotNet/CILEmitter.hpp>
#include <IgniLang/Compiler/DotNet/TypeMapper.hpp>
#include <IgniLang/Semantic/SemanticAnalyzer.hpp>

#include <optional>
#include <ostream>
#include <sstream>
#include <vector>

namespace igni
{

class DotNetCodeGenerator final
	: public ast::BaseAstVisitor
	, dotnet::TypeMapper
	, dotnet::CILEmitter
{
	template <std::size_t N>
	using HashedStringMap = re::flat_map<re::HashedString, std::string_view, N>;

	static constexpr auto TYPE_OBJECT = "class [mscorlib]System.Object";
	static constexpr auto TYPE_VOID = "void";
	static constexpr auto ANNO_BASE_CLASS = "DotNetBaseClass";

public:
	DotNetCodeGenerator(std::ostream& out, const sem::SemanticAnalyzer& semantics)
		: CILEmitter(out)
		, m_out(out)
		, m_semanticAnalyzer(semantics)
	{
	}

	void Generate(const ast::Program* program)
	{
		auto scanAnnotations = [&](const std::vector<ast::Annotation>& annotations) {
			for (const auto& anno : annotations)
			{
				if (const auto strArg = ast::AnnotationUtils::GetAnnotationStringArg(anno))
				{
					ExtractAssemblies(*strArg);
				}
			}
		};

		for (const auto& stmt : program->statements)
		{
			if (const auto decl = dynamic_cast<const ast::Decl*>(stmt.get()))
			{
				scanAnnotations(decl->annotations);

				if (const auto classDecl = dynamic_cast<const ast::ClassDecl*>(decl))
				{
					for (const auto& member : classDecl->members)
					{
						scanAnnotations(member->annotations);
					}
				}
			}
		}

		m_out << "// Auto-generated .NET CIL\n";
		m_out << ".assembly IgniProgram { }\n";
		m_out << ".assembly extern mscorlib { }\n\n";

		for (const auto& asmName : m_externAssemblies)
		{
			m_out << ".assembly extern " << asmName << " { }\n";
		}
		m_out << "\n";

		m_out << ".class public auto ansi beforefieldinit IgniGlobalModule extends [mscorlib]System.Object\n{\n";
		m_currentClass = nullptr;

		for (const auto& stmt : program->statements)
		{
			if (const auto varDecl = dynamic_cast<const ast::VarDecl*>(stmt.get()))
			{
				re::String cilType = "class [mscorlib]System.Object";
				if (varDecl->initializer)
				{
					if (const auto semType = m_semanticAnalyzer.GetBindings().GetExpressionType(varDecl->initializer.get()))
					{
						cilType = MapToCIL(semType->name);
					}
				}
				m_globalVars[varDecl->name] = cilType;
				m_out << "  .field public static " << cilType << " '" << varDecl->name << "'\n";
			}
			else if (const auto valDecl = dynamic_cast<const ast::ValDecl*>(stmt.get()))
			{
				re::String cilType = "class [mscorlib]System.Object";
				if (valDecl->initializer)
				{
					if (const auto semType = m_semanticAnalyzer.GetBindings().GetExpressionType(valDecl->initializer.get()))
					{
						cilType = MapToCIL(semType->name);
					}
				}
				m_globalVars[valDecl->name] = cilType;
				m_out << "  .field public static " << cilType << " '" << valDecl->name << "'\n";
			}
		}

		if (!m_globalVars.empty())
		{
			m_out << "  .method private hidebysig specialname rtspecialname static void .cctor() cil managed\n  {\n    .maxstack 8\n";
			m_currentOut = &m_out;
			for (const auto& stmt : program->statements)
			{
				if (const auto varDecl = dynamic_cast<const ast::VarDecl*>(stmt.get()))
				{
					if (varDecl->initializer)
					{
						varDecl->initializer->Accept(*this);
						m_out << "    stsfld " << m_globalVars[varDecl->name] << " IgniGlobalModule::" << varDecl->name << "\n";
					}
				}
				else if (const auto valDecl = dynamic_cast<const ast::ValDecl*>(stmt.get()))
				{
					if (valDecl->initializer)
					{
						valDecl->initializer->Accept(*this);
						m_out << "    stsfld " << m_globalVars[valDecl->name] << " IgniGlobalModule::" << valDecl->name << "\n";
					}
				}
			}
			m_out << "    ret\n  }\n\n";
		}

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

		for (const auto& lambda : m_lambdas)
		{
			GenerateLambdaClass(lambda);
		}
	}

	void Visit(const ast::ClassDecl* node) override
	{
		if (node->isExternal || !node->typeParams.empty())
		{
			return;
		}

		m_currentClass = node;

		const re::String baseClass = GetBaseClass(node->annotations);
		m_out << ".class public auto ansi beforefieldinit " << node->name << " extends " << baseClass << "\n{\n";

		if (const auto semClass = m_semanticAnalyzer.GetClassType(node->name))
		{
			for (const auto& [fieldName, fieldInfo] : semClass->fields)
			{
				m_out << "  .field public " << MapToCIL(fieldInfo.type->name) << " '" << fieldName << "'\n";
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

		const re::String baseClass = GetBaseClass(m_currentClass->annotations);
		const auto baseCtor = baseClass + "::.ctor()";

		*m_currentOut << "    ldarg.0\n    call instance void " << baseCtor << "\n";

		for (const auto& member : m_currentClass->members)
		{
			if (const auto varDecl = dynamic_cast<const ast::VarDecl*>(member.get()))
			{
				if (varDecl->initializer)
				{
					*m_currentOut << "    ldarg.0 // this\n";
					varDecl->initializer->Accept(*this);
					const auto semType = m_semanticAnalyzer.GetBindings().GetExpressionType(varDecl->initializer.get());
					*m_currentOut << "    stfld " << MapToCIL(semType->name) << " " << m_currentClass->name << "::" << varDecl->name << "\n";
				}
			}
			else if (const auto valDecl = dynamic_cast<const ast::ValDecl*>(member.get()))
			{
				if (valDecl->initializer)
				{
					*m_currentOut << "    ldarg.0 // this\n";
					valDecl->initializer->Accept(*this);
					const auto semType = m_semanticAnalyzer.GetBindings().GetExpressionType(valDecl->initializer.get());
					*m_currentOut << "    stfld " << MapToCIL(semType->name) << " " << m_currentClass->name << "::" << valDecl->name << "\n";
				}
			}
		}

		const re::String sig = ".method public hidebysig specialname rtspecialname instance void .ctor(" + BuildParamSignature(node->parameters) + ") cil managed";
		EmitMethodBody(sig, node->body.get(), false);
	}

	void Visit(const ast::FunDecl* node) override
	{
		if (node->isExternal || !node->typeParams.empty())
		{
			return;
		}

		PrepareMethodScope(m_currentClass != nullptr, node->parameters);

		const re::String retType = node->returnType ? MapAstType(node->returnType.get()) : TYPE_VOID;
		const re::String instanceKw = m_currentClass ? "instance " : "static ";
		const bool isEntryPoint = (node->name == "main" && !m_currentClass);

		re::String methodName = isEntryPoint ? "main" : m_semanticAnalyzer.GetBindings().GetMangledName(node);

		const re::String sig = ".method public hidebysig " + instanceKw + retType + " " + methodName + "(" + BuildParamSignature(node->parameters, node->isVararg) + ") cil managed";
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
		if (m_currentOut == &m_out)
		{
			return;
		}

		DeclareLocal(node->name, node->initializer.get());
	}

	void Visit(const ast::ValDecl* node) override
	{
		if (m_currentOut == &m_out)
		{
			return;
		}

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
		else if (const auto idxAccess = dynamic_cast<const ast::IndexExpr*>(node->target.get()))
		{
			idxAccess->array->Accept(*this);
			idxAccess->index->Accept(*this);
			*m_currentOut << "    conv.i4\n";

			if (node->value)
			{
				node->value->Accept(*this);
			}

			re::String elemType = "System.Object";
			if (const auto arrSemType = m_semanticAnalyzer.GetBindings().GetExpressionType(idxAccess->array.get()))
			{
				if (const auto classType = std::dynamic_pointer_cast<sem::ClassType>(arrSemType))
				{
					if (!classType->typeArguments.empty())
					{
						elemType = classType->typeArguments[0]->name;
					}
				}
			}
			*m_currentOut << "    stelem." << GetElemSuffix(elemType) << "\n";
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
		case TokenType::KwNull:      *m_currentOut << "    ldnull\n"; break;
		case TokenType::KwTrue:      *m_currentOut << "    ldc.i4.1\n"; break;
		case TokenType::KwFalse:     *m_currentOut << "    ldc.i4.0\n"; break;
		default: break;
        } // clang-format on
	}

	void Visit(const ast::UnaryExpr* node) override
	{
		if (node->op == "-")
		{
			node->operand->Accept(*this);
			*m_currentOut << "    neg\n";
		}
		else if (node->op == "!" || node->op == "not")
		{
			node->operand->Accept(*this);
			*m_currentOut << "    ldc.i4.0\n    ceq\n";
		}
		else if (node->op == "++" || node->op == "--")
		{
			if (const auto id = dynamic_cast<const ast::IdentifierExpr*>(node->operand.get()))
			{
				EmitIdentifierAccess(id, false, nullptr);

				*m_currentOut << "    dup\n";
				*m_currentOut << "    ldc.i8 1\n";
				*m_currentOut << (node->op == "++" ? "    add\n" : "    sub\n");

				EmitIdentifierAccess(id, true, nullptr);
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

		static constexpr HashedStringMap BinaryOpMap = { {
			{ "+"_hs, "    add\n" },
			{ "-"_hs, "    sub\n" },
			{ "*"_hs, "    mul\n" },
			{ "/"_hs, "    div\n" },
			{ "%"_hs, "    rem\n" },
			{ ">"_hs, "    cgt\n" },
			{ "<"_hs, "    clt\n" },
			{ "=="_hs, "    ceq\n" },
			{ "||"_hs, "    or\n" },
			{ "&&"_hs, "    and\n" },
			{ "or"_hs, "    or\n" },
			{ "and"_hs, "    and\n" },
			{ "!="_hs, "    ceq\n    ldc.i4.0\n    ceq\n" },
			{ "<="_hs, "    cgt\n    ldc.i4.0\n    ceq\n" },
			{ ">="_hs, "    clt\n    ldc.i4.0\n    ceq\n" },
		} };

		if (const auto instruction = BinaryOpMap[node->op.Hashed()])
		{
			*m_currentOut << *instruction;
		}
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
			const std::size_t currentLabel = m_labelCount++;
			const std::string startLabel = "L_foreach_start_" + std::to_string(currentLabel);
			const std::string endLabel = "L_foreach_end_" + std::to_string(currentLabel);

			const std::string arrName = "_arr_" + std::to_string(currentLabel);
			DeclareLocal(arrName, node->startExpr.get());
			const int arrIdx = GetLocalIndex(arrName);

			const std::string idxName = "_idx_" + std::to_string(currentLabel);
			m_locals.emplace_back(idxName);
			m_localTypes.emplace_back("int64");
			const int idxIdx = static_cast<int>(m_locals.size() - 1);
			*m_currentOut << "    ldc.i8 0\n    stloc " << idxIdx << "\n";

			re::String elemCilType = "class [mscorlib]System.Object";
			re::String pureSemType = "System.Object";

			if (const auto collSemType = m_semanticAnalyzer.GetBindings().GetExpressionType(node->startExpr.get()))
			{
				if (const auto classType = std::dynamic_pointer_cast<sem::ClassType>(collSemType))
				{
					if (!classType->typeArguments.empty())
					{
						pureSemType = classType->typeArguments[0]->name;
						elemCilType = MapToCIL(pureSemType);
					}
				}
			}

			m_locals.push_back(node->iteratorName);
			m_localTypes.push_back(elemCilType);
			const int iterIdx = static_cast<int>(m_locals.size() - 1);

			*m_currentOut << startLabel << ":\n";

			*m_currentOut << "    ldloc " << idxIdx << "\n";
			*m_currentOut << "    ldloc " << arrIdx << "\n";
			*m_currentOut << "    ldlen\n    conv.i8\n";
			*m_currentOut << "    bge " << endLabel << "\n";

			*m_currentOut << "    ldloc " << arrIdx << "\n";
			*m_currentOut << "    ldloc " << idxIdx << "\n";
			*m_currentOut << "    conv.i4\n";
			*m_currentOut << "    ldelem." << GetElemSuffix(pureSemType) << "\n";
			*m_currentOut << "    stloc " << iterIdx << " // " << node->iteratorName << "\n";

			if (node->body)
			{
				node->body->Accept(*this);
			}

			*m_currentOut << "    ldloc " << idxIdx << "\n";
			*m_currentOut << "    ldc.i8 1\n";
			*m_currentOut << "    add\n";
			*m_currentOut << "    stloc " << idxIdx << "\n";

			*m_currentOut << "    br " << startLabel << "\n";

			*m_currentOut << endLabel << ":\n";
			return;
		}

		const std::string startLabel = "L_for_start_" + std::to_string(m_labelCount);
		const std::string endLabel = "L_for_end_" + std::to_string(m_labelCount++);

		DeclareLocal(node->iteratorName, node->startExpr.get());
		const int iterIdx = GetLocalIndex(node->iteratorName);

		const std::string limitName = "_for_limit_" + std::to_string(m_labelCount);
		DeclareLocal(limitName, node->endExpr.get());
		const int limitIdx = GetLocalIndex(limitName);

		*m_currentOut << startLabel << ":\n"
					  << "    ldloc " << iterIdx << "\n"
					  << "    ldloc " << limitIdx << "\n"
					  << "    bgt " << endLabel << "\n";

		if (node->body)
		{
			node->body->Accept(*this);
		}

		*m_currentOut << "    ldloc " << iterIdx << "\n    ldc.i8 1\n    add\n    stloc " << iterIdx << "\n"
					  << "    br " << startLabel << "\n"
					  << endLabel << ":\n";
	}

	void Visit(const ast::CallExpr* node) override
	{
		const auto& callInfo = m_semanticAnalyzer.GetBindings().callInfo.at(node);
		const auto& targetAnnos = callInfo.target->annotations;

		auto emitArgs = [&](const bool ignoreVararg = false) {
			if (callInfo.target->isVararg && !ignoreVararg)
			{
				const std::size_t normalCount = callInfo.target->paramTypes.size() - 1;

				for (std::size_t i = 0; i < normalCount; ++i)
				{
					if (node->arguments[i])
					{
						node->arguments[i]->Accept(*this);
					}
				}

				const std::size_t varargCount = node->arguments.size() - normalCount;
				*m_currentOut << "    ldc.i4 " << varargCount << "\n";

				const re::String elemType = callInfo.target->paramTypes.back()->name;
				re::String cilElemType = MapToCIL(elemType);
				if (cilElemType.Find("[]") != re::String::NPos)
				{
					cilElemType = cilElemType.Substring(0, cilElemType.Length() - 2);
				}

				*m_currentOut << "    newarr " << cilElemType << "\n";

				for (std::size_t i = 0; i < varargCount; ++i)
				{
					*m_currentOut << "    dup\n";
					*m_currentOut << "    ldc.i4 " << i << "\n";

					if (node->arguments[normalCount + i])
					{
						node->arguments[normalCount + i]->Accept(*this);

						if (elemType == "Any" || elemType == "System.Object")
						{
							if (const auto argSemType = m_semanticAnalyzer.GetBindings().GetExpressionType(node->arguments[normalCount + i].get()))
							{
								if (argSemType->name == "System.Int64")
								{
									*m_currentOut << "    box [mscorlib]System.Int64\n";
								}
								else if (argSemType->name == "System.Double")
								{
									*m_currentOut << "    box [mscorlib]System.Double\n";
								}
								else if (argSemType->name == "System.Boolean")
								{
									*m_currentOut << "    box [mscorlib]System.Boolean\n";
								}
							}
						}
					}

					*m_currentOut << "    stelem." << GetElemSuffix(elemType) << "\n";
				}
			}
			else
			{
				for (const auto& arg : node->arguments)
					if (arg)
					{
						arg->Accept(*this);
					}
			}
		};

		if (const auto inlineOp = ast::AnnotationUtils::GetAnnotationArg(targetAnnos, "DotNetOpcode"); inlineOp)
		{
			if (const auto& op = *inlineOp; op == "ldlen")
			{
				if (const auto memAccess = dynamic_cast<const ast::MemberAccessExpr*>(node->callee.get()))
				{
					memAccess->object->Accept(*this);
				}
				*m_currentOut << "    ldlen\n    conv.i8\n";
			}
			else if (op == "ldelem")
			{
				if (const auto memAccess = dynamic_cast<const ast::MemberAccessExpr*>(node->callee.get()))
				{
					memAccess->object->Accept(*this);
				}
				if (!node->arguments.empty() && node->arguments[0])
				{
					node->arguments[0]->Accept(*this);
				}
				*m_currentOut << "    conv.i4\n";

				re::String elemType = "System.Object";
				if (const auto classType = std::dynamic_pointer_cast<sem::ClassType>(callInfo.target->paramTypes[0]))
				{
					if (!classType->typeArguments.empty())
					{
						elemType = classType->typeArguments[0]->name;
					}
				}
				*m_currentOut << "    ldelem." << GetElemSuffix(elemType) << "\n";
			}
			else if (op == "stelem")
			{
				if (const auto memAccess = dynamic_cast<const ast::MemberAccessExpr*>(node->callee.get()))
				{
					memAccess->object->Accept(*this);
				}
				if (!node->arguments.empty() && node->arguments[0])
				{
					node->arguments[0]->Accept(*this);
				}
				*m_currentOut << "    conv.i4\n";

				if (node->arguments.size() > 1 && node->arguments[1])
				{
					node->arguments[1]->Accept(*this);
				}

				re::String elemType = "System.Object";
				if (const auto classType = std::dynamic_pointer_cast<sem::ClassType>(callInfo.target->paramTypes[0]))
				{
					if (!classType->typeArguments.empty())
					{
						elemType = classType->typeArguments[0]->name;
					}
				}
				*m_currentOut << "    stelem." << GetElemSuffix(elemType) << "\n";
			}
			else if (op == "newarr")
			{
				emitArgs(true);
				re::String elemType = "class [mscorlib]System.Object";
				if (const auto classType = std::dynamic_pointer_cast<sem::ClassType>(callInfo.target->returnType))
				{
					if (!classType->typeArguments.empty())
					{
						elemType = MapToCIL(classType->typeArguments[0]->name);
					}
				}
				*m_currentOut << "    newarr " << elemType << "\n";
			}
			else
			{
				emitArgs();
				*m_currentOut << "    " << op << "\n";
			}
			return;
		}

		if (callInfo.dispatchMode == CallDispatchType::Indirect)
		{
			if (node->callee)
			{
				node->callee->Accept(*this);
			}
			emitArgs();
			const re::String retType = callInfo.target && callInfo.target->returnType ? MapToCIL(callInfo.target->returnType->name) : TYPE_VOID;
			*m_currentOut << "    callvirt instance " << retType << " " << callInfo.target->name << "::Invoke(" << BuildTypeSignature(callInfo.target->paramTypes, 0, callInfo.target->isVararg) << ")\n";

			return;
		}

		if (callInfo.dispatchMode == CallDispatchType::Native)
		{
			if (const auto dotnetMethod = ast::AnnotationUtils::GetAnnotationArg(targetAnnos, "DotNetMethod"))
			{
				emitArgs(true);
				*m_currentOut << "    call " << *dotnetMethod << "\n";
				return;
			}
			emitArgs();
			*m_currentOut << "    call " << callInfo.target->name << "\n";
			return;
		}

		if (callInfo.isConstructorCall)
		{
			emitArgs();
			*m_currentOut << "    newobj instance void " << callInfo.mangledClassName << "::.ctor("
						  << BuildTypeSignature(callInfo.target->paramTypes, 1, callInfo.target->isVararg) << ")\n";
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
			*m_currentOut << "    callvirt instance " << MapToCIL(callInfo.target->returnType->name)
						  << " " << callInfo.target->paramTypes[0]->name << "::" << callInfo.asmLabel
						  << "(" << BuildTypeSignature(callInfo.target->paramTypes, 1, callInfo.target->isVararg) << ")\n";

			return;
		}

		emitArgs();
		const re::String retType = callInfo.target && callInfo.target->returnType
			? MapToCIL(callInfo.target->returnType->name)
			: TYPE_VOID;

		re::String finalAsmLabel = callInfo.asmLabel;
		if (const std::size_t firstAt = finalAsmLabel.Find('@'); firstAt != re::String::NPos)
		{
			if (const std::size_t secondAt = finalAsmLabel.Find('@', firstAt + 1); secondAt != re::String::NPos)
			{
				finalAsmLabel = finalAsmLabel.Substring(0, secondAt);
			}
		}

		*m_currentOut << "    call " << retType << " IgniGlobalModule::" << finalAsmLabel
					  << "(" << BuildTypeSignature(callInfo.target->paramTypes, 0, callInfo.target->isVararg) << ")\n";
	}

	void Visit(const ast::LambdaExpr* node) override
	{
		const auto semType = m_semanticAnalyzer.GetBindings().GetExpressionType(node);
		re::String lambdaClassName = semType->name;

		std::vector<re::String> capTypes;
		for (const auto& capName : node->captures)
		{
			re::String cilType = "class [mscorlib]System.Object";

			if (m_globalVars.contains(capName))
			{
				cilType = m_globalVars.at(capName);
				*m_currentOut << "    ldsfld " << cilType << " IgniGlobalModule::" << capName << "\n";
			}
			else if (const int locIdx = GetLocalIndex(capName); locIdx != -1)
			{
				cilType = m_localTypes[locIdx];
				*m_currentOut << "    ldloc " << locIdx << "\n";
			}
			else if (const int argIdx = GetArgIndex(capName); argIdx != -1)
			{
				cilType = m_argTypes[argIdx];
				*m_currentOut << "    ldarg " << argIdx << "\n";
			}

			capTypes.push_back(cilType);
		}

		*m_currentOut << "    newobj instance void " << lambdaClassName << "::.ctor(";
		for (size_t i = 0; i < capTypes.size(); ++i)
		{
			*m_currentOut << capTypes[i] << (i < capTypes.size() - 1 ? ", " : "");
		}
		*m_currentOut << ")\n";

		m_lambdas.emplace_back(node, lambdaClassName, capTypes);
	}

	void Visit(const ast::ExprStmt* node) override
	{
		if (node->expr)
		{
			node->expr->Accept(*this);

			if (!dynamic_cast<const ast::AssignExpr*>(node->expr.get()))
			{
				if (const auto semType = m_semanticAnalyzer.GetBindings().GetExpressionType(node->expr.get()))
				{
					if (semType->name != "Unit" && semType->name != "System.Void")
					{
						*m_currentOut << "    pop\n";
					}
				}
			}
		}
	}

	void Visit(const ast::ReturnStmt* node) override
	{
		if (node->expr)
		{
			node->expr->Accept(*this);
		}
		*m_currentOut << "    ret\n";
	}

	void Visit(const ast::IndexExpr* node) override
	{
		node->array->Accept(*this);
		node->index->Accept(*this);
		*m_currentOut << "    conv.i4\n";

		re::String elemType = "System.Object";
		if (const auto arrSemType = m_semanticAnalyzer.GetBindings().GetExpressionType(node->array.get()))
		{
			if (const auto classType = std::dynamic_pointer_cast<sem::ClassType>(arrSemType))
			{
				if (!classType->typeArguments.empty())
				{
					elemType = classType->typeArguments[0]->name;
				}
			}
		}
		*m_currentOut << "    ldelem." << GetElemSuffix(elemType) << "\n";
	}

	void Visit(const ast::TypeCastExpr* node) override
	{
		node->expr->Accept(*this);

		if (m_semanticAnalyzer.GetBindings().castTargets.contains(node))
		{
			const re::String targetType = m_semanticAnalyzer.GetBindings().castTargets.at(node);
			*m_currentOut << "    castclass " << MapToCIL(targetType) << "\n";
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

	std::vector<re::String> m_externAssemblies;

	std::unordered_map<re::String, re::String> m_globalVars;

	struct LambdaData
	{
		const ast::LambdaExpr* node;
		re::String className;
		std::vector<re::String> captureTypes;
	};

	std::vector<LambdaData> m_lambdas;
	const ast::LambdaExpr* m_currentLambda = nullptr;
	const LambdaData* m_currentLambdaData = nullptr;
	std::vector<re::String> m_argTypes;

	void PrepareMethodScope(const bool hasThis, const std::vector<ast::Parameter>& params)
	{
		m_locals.clear();
		m_localTypes.clear();
		m_args.clear();
		m_argTypes.clear();

		if (hasThis)
		{
			m_args.emplace_back("this");
			m_argTypes.emplace_back(m_currentClass ? "class " + m_currentClass->name : "class [mscorlib]System.Object");
		}
		for (const auto& p : params)
		{
			m_args.push_back(p.name);
			m_argTypes.push_back(MapAstType(p.type.get()));
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
				*m_currentOut << "    " << (isStore ? "stfld " : "ldfld ") << MapToCIL(fieldType->name)
							  << " " << classType->name << "::" << member << "\n";
			}
		}
	}

	void EmitIdentifierAccess(const ast::IdentifierExpr* id, const bool isStore, const ast::Expr* assignValue)
	{
		if (m_currentLambdaData)
		{
			for (size_t i = 0; i < m_currentLambdaData->node->captures.size(); ++i)
			{
				if (m_currentLambdaData->node->captures[i] == id->name)
				{
					const re::String cilType = m_currentLambdaData->captureTypes[i];
					*m_currentOut << "    ldarg.0 // this (lambda capture)\n";
					if (assignValue)
					{
						assignValue->Accept(*this);
					}
					*m_currentOut << "    " << (isStore ? "stfld " : "ldfld ") << cilType << " " << m_currentLambdaData->className << "::" << id->name << "\n";

					return;
				}
			}
		}

		if (m_semanticAnalyzer.GetBindings().implicitThisNames.contains(id))
		{
			*m_currentOut << "    ldarg.0 // this\n";
			if (assignValue)
			{
				assignValue->Accept(*this);
			}

			const auto classType = m_semanticAnalyzer.GetClassType(m_currentClass->name);
			const auto fieldType = classType->fields.at(id->name).type;
			*m_currentOut << "    " << (isStore ? "stfld " : "ldfld ") << MapToCIL(fieldType->name)
						  << " " << classType->name << "::" << id->name << "\n";

			return;
		}

		if (m_globalVars.contains(id->name))
		{
			*m_currentOut << "    " << (isStore ? "stsfld " : "ldsfld ") << m_globalVars.at(id->name) << " IgniGlobalModule::" << id->name << "\n";
			return;
		}

		if (const int locIdx = GetLocalIndex(id->name); locIdx != -1)
		{
			*m_currentOut << "    " << (isStore ? "stloc " : "ldloc ") << locIdx << " // " << id->name << "\n";
			return;
		}
		if (const int argIdx = GetArgIndex(id->name); argIdx != -1)
		{
			*m_currentOut << "    " << (isStore ? "starg " : "ldarg ") << argIdx << " // " << id->name << "\n";
		}
	}

	void DeclareLocal(const re::String& name, const ast::Expr* initExpr)
	{
		re::String cilType = "int64";
		if (initExpr)
		{
			if (const auto semType = m_semanticAnalyzer.GetBindings().GetExpressionType(initExpr))
			{
				cilType = MapToCIL(semType->name);
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

		*m_currentOut << "    stloc " << idx << " // " << name << "\n";
	}

	[[nodiscard]] static re::String BuildTypeSignature(
		const std::vector<std::shared_ptr<sem::SemanticType>>& types,
		const std::size_t startIndex,
		const bool isVararg = false)
	{
		re::String sig;
		for (std::size_t i = startIndex; i < types.size(); ++i)
		{
			re::String typeSig = MapToCIL(types[i]->name);

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

	[[nodiscard]] static re::String GetBaseClass(const std::vector<ast::Annotation>& annotations)
	{
		for (const auto& anno : annotations)
		{
			if (anno.name == ANNO_BASE_CLASS)
			{
				if (const auto strArg = ast::AnnotationUtils::GetAnnotationStringArg(anno))
				{
					return *strArg;
				}
			}
		}

		return "[mscorlib]System.Object";
	}

	void ExtractAssemblies(const re::String& signature)
	{
		std::size_t start = 0;
		while ((start = signature.Find('[', start)) != re::String::NPos)
		{
			const std::size_t end = signature.Find(']', start);
			if (end == std::string::npos)
			{
				break;
			}

			if (const re::String asmName = signature.Substring(start + 1, end - start - 1);
				asmName != "mscorlib" && std::ranges::find(m_externAssemblies, asmName) == m_externAssemblies.end())
			{
				m_externAssemblies.push_back(asmName);
			}
			start = end + 1;
		}
	}

	template <typename Container, typename Value>
	[[nodiscard]] static int IndexOf(const Container& container, const Value& value)
	{
		const auto it = std::ranges::find(container, value);

		return it != container.end()
			? static_cast<int>(std::distance(container.begin(), it))
			: -1;
	}

	[[nodiscard]] int GetLocalIndex(const re::String& name) const
	{
		return IndexOf(m_locals, name);
	}

	[[nodiscard]] int GetArgIndex(const re::String& name) const
	{
		return IndexOf(m_args, name);
	}

	void GenerateLambdaClass(const LambdaData& data)
	{
		m_out << ".class public auto ansi beforefieldinit " << data.className << " extends [mscorlib]System.Object\n{\n";

		for (std::size_t i = 0; i < data.captureTypes.size(); ++i)
		{
			m_out << "  .field public " << data.captureTypes[i] << " '" << data.node->captures[i] << "'\n";
		}

		m_out << "  .method public hidebysig specialname rtspecialname instance void .ctor(";
		for (std::size_t i = 0; i < data.captureTypes.size(); ++i)
		{
			m_out << data.captureTypes[i] << (i < data.captureTypes.size() - 1 ? ", " : "");
		}
		m_out << ") cil managed\n  {\n    .maxstack 8\n    ldarg.0\n    call instance void [mscorlib]System.Object::.ctor()\n";

		for (std::size_t i = 0; i < data.captureTypes.size(); ++i)
		{
			m_out << "    ldarg.0\n    ldarg " << (i + 1) << "\n    stfld " << data.captureTypes[i] << " " << data.className << "::" << data.node->captures[i] << "\n";
		}
		m_out << "    ret\n  }\n\n";

		m_currentLambdaData = &data;
		PrepareMethodScope(true, data.node->parameters);

		const re::String retType = data.node->returnType ? MapAstType(data.node->returnType.get()) : TYPE_VOID;
		const re::String sig = ".method public hidebysig instance " + retType + " Invoke(" + BuildParamSignature(data.node->parameters, false) + ") cil managed";
		EmitMethodBody(sig, data.node->body.get(), false);

		m_currentLambdaData = nullptr;
		m_out << "}\n\n";
	}
};

} // namespace igni