#pragma once

#include <Core/flat_map.hpp>
#include <IgniLang/AST/AstNodes.hpp>
#include <IgniLang/AST/Utils/AnnotationUtils.hpp>
#include <IgniLang/Compiler/DotNet/CILEmitter.hpp>
#include <IgniLang/Compiler/DotNet/LambdaHelper.hpp>
#include <IgniLang/Compiler/DotNet/TypeMapper.hpp>
#include <IgniLang/Semantic/Helpers/NameMangler.hpp>
#include <IgniLang/Semantic/SemanticAnalyzer.hpp>

#include <algorithm>
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
		, m_lambdaHelper(*this, semantics)
	{
	}

	void Generate(const ast::Program* program)
	{
		auto scanAnnotations = [&](const std::vector<std::unique_ptr<ast::AnnotationNode>>& annotations) {
			for (const auto& anno : annotations)
			{
				if (const auto strArg = ast::AnnotationUtils::GetAnnotationStringArg(anno.get()))
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
			SetOutStream(m_out);
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
			Ret();
		}
		SetOutStream(m_out);

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

		m_lambdaHelper.GenerateDelegates(m_out);

		m_lambdaHelper.GenerateAllClasses(m_out, [&](const re::String& sig, const ast::Block* body, const bool isEntry, const std::vector<std::unique_ptr<ast::ParameterNode>>& params) {
			PrepareMethodScope(true, params, body);
			EmitMethodBody(sig, body, isEntry);
		});
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

		PrepareMethodScope(true, node->parameters, node->body.get());

		if (const re::String baseClass = GetBaseClass(m_currentClass->annotations); baseClass == "[mscorlib]System.Object")
		{
			LdArg0();
			Call("instance void [mscorlib]System.Object::.ctor()");
		}

		for (const auto& member : m_currentClass->members)
		{
			if (const auto varDecl = dynamic_cast<const ast::VarDecl*>(member.get()))
			{
				if (varDecl->initializer)
				{
					LdArg0();
					varDecl->initializer->Accept(*this);
					const auto semType = m_semanticAnalyzer.GetBindings().GetExpressionType(varDecl->initializer.get());
					StFld(MapToCIL(semType->name), m_currentClass->name, varDecl->name);
				}
			}
			else if (const auto valDecl = dynamic_cast<const ast::ValDecl*>(member.get()))
			{
				if (valDecl->initializer)
				{
					LdArg0();
					valDecl->initializer->Accept(*this);
					const auto semType = m_semanticAnalyzer.GetBindings().GetExpressionType(valDecl->initializer.get());
					StFld(MapToCIL(semType->name), m_currentClass->name, valDecl->name);
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

		std::shared_ptr<sem::FunctionType> funType = nullptr;
		re::String methodName = (node->name == "main" && !m_currentClass) ? "main" : m_semanticAnalyzer.GetBindings().GetMangledName(node);

		if (m_currentClass)
		{
			methodName = node->name;
			if (const auto semClass = m_semanticAnalyzer.GetClassType(m_currentClass->name))
			{
				if (semClass->methods.contains(node->name))
				{
					if (const auto fg = std::dynamic_pointer_cast<sem::FunctionGroup>(semClass->methods.at(node->name)))
					{
						for (const auto& overload : fg->overloads)
						{
							if (overload->paramTypes.size() == node->parameters.size() + 1)
							{
								funType = overload;
								break;
							}
						}
					}
					else
					{
						funType = std::dynamic_pointer_cast<sem::FunctionType>(semClass->methods.at(node->name));
					}
				}
			}
		}
		else
		{
			auto environment = m_semanticAnalyzer.Env();
			if (const auto* sym = environment.Resolve(node->name))
			{
				if (const auto fg = std::dynamic_pointer_cast<sem::FunctionGroup>(sym->type))
				{
					for (const auto& overload : fg->overloads)
					{
						if (overload->paramTypes.size() == node->parameters.size())
						{
							funType = overload;
							break;
						}
					}
				}
				else
				{
					funType = std::dynamic_pointer_cast<sem::FunctionType>(sym->type);
				}
			}

			if (!funType)
			{
				for (const auto& [callExpr, info] : m_semanticAnalyzer.GetBindings().callInfo)
				{
					if (info.asmLabel == methodName)
					{
						funType = info.target;
						break;
					}
				}
			}
		}

		const bool isInstanceMethod = (m_currentClass != nullptr);
		bool hasImplicitThis = false;

		if (funType && funType->paramTypes.size() > node->parameters.size() && !isInstanceMethod)
		{
			hasImplicitThis = true;
		}

		PrepareMethodScope(isInstanceMethod, node->parameters, node->body.get());

		if (hasImplicitThis)
		{
			m_args.insert(m_args.begin(), "this");
			m_argTypes.insert(m_argTypes.begin(), MapToCIL(funType->paramTypes[0]->name));
		}

		re::String sig;
		if (funType)
		{
			sig = BuildCilSignature(funType.get(), "", methodName, isInstanceMethod, isInstanceMethod);
			if (!isInstanceMethod)
			{
				sig = "static " + sig;
			}
		}
		else
		{
			const re::String retType = node->returnType ? MapAstType(node->returnType.get()) : TYPE_VOID;
			sig = (isInstanceMethod ? "instance " : "static ") + retType + " " + methodName + "(" + BuildParamSignature(node->parameters, node->isVararg) + ")";
		}

		sig = ".method public hidebysig " + sig + " cil managed";
		EmitMethodBody(sig, node->body.get(), node->name == "main" && !m_currentClass);
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
		if (m_isWritingGlobal)
		{
			return;
		}
		DeclareLocal(node->name, node->initializer.get());
	}

	void Visit(const ast::ValDecl* node) override
	{
		if (m_isWritingGlobal)
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
			EmitIdentifierAccess(id, true, node->value.get());
		}
		else if (const auto idxAccess = dynamic_cast<const ast::IndexExpr*>(node->target.get()))
		{
			idxAccess->array->Accept(*this);
			idxAccess->index->Accept(*this);
			Emit("conv.i4");
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
			Emit("stelem." + GetElemSuffix(elemType));
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
		const auto exprType = m_semanticAnalyzer.GetBindings().GetExpressionType(node);

		if (const auto funType = std::dynamic_pointer_cast<sem::FunctionType>(exprType))
		{
			if (!m_globalVars.contains(node->name) && GetLocalIndex(node->name) == -1 && GetArgIndex(node->name) == -1)
			{
				RegisterDelegateFromType(funType);
				const re::String delegateName = GetDelegateName(funType.get());

				std::vector<re::String> typeNames;
				for (const auto& pt : funType->paramTypes)
				{
					typeNames.push_back(pt->name);
				}

				re::String mangledName = sem::NameMangler::Mangle(node->name, typeNames, false);
				const re::String targetMethodSig = BuildCilSignature(funType.get(), "IgniGlobalModule", mangledName, false, false);

				LdNull();
				Emit("ldftn " + targetMethodSig);
				NewObj(delegateName + "::.ctor(object, native int)");
				return;
			}
		}

		EmitIdentifierAccess(node, false, nullptr);
	}

	void Visit(const ast::LiteralExpr* node) override
	{
		switch (node->token.type)
		{ // clang-format off
		case TokenType::IntConst: LdcI8(std::stoll(std::string(node->token.lexeme))); break;
		case TokenType::FloatConst: Emit(re::String("ldc.r8 ") + node->token.lexeme); break;
		case TokenType::StringConst: Emit(re::String("ldstr ") + node->token.lexeme); break;
		case TokenType::KwNull: Emit("ldnull"); break;
		case TokenType::KwTrue: LdcI4_True(); break;
		case TokenType::KwFalse: LdcI4_False(); break;
		default: break;
		} // clang-format on
	}

	void Visit(const ast::UnaryExpr* node) override
	{
		if (node->op == "-")
		{
			node->operand->Accept(*this);
			Emit("neg");
		}
		else if (node->op == "!" || node->op == "not")
		{
			node->operand->Accept(*this);
			LdcI4_False();
			Emit("ceq");
		}
		else if (node->op == "++" || node->op == "--")
		{
			if (const auto id = dynamic_cast<const ast::IdentifierExpr*>(node->operand.get()))
			{
				EmitIdentifierAccess(id, false, nullptr);
				Emit("dup");
				LdcI8(1);
				Emit(node->op == "++" ? "add" : "sub");
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
			{ "+"_hs, "add" },
			{ "-"_hs, "sub" },
			{ "*"_hs, "mul" },
			{ "/"_hs, "div" },
			{ "%"_hs, "rem" },
			{ ">"_hs, "cgt" },
			{ "<"_hs, "clt" },
			{ "=="_hs, "ceq" },
			{ "||"_hs, "or" },
			{ "&&"_hs, "and" },
			{ "or"_hs, "or" },
			{ "and"_hs, "and" },
		} };

		if (const auto instruction = BinaryOpMap[node->op.Hashed()])
		{
			Emit(*instruction);
		}
		else if (node->op == "!=")
		{
			Emit("ceq");
			LdcI4_False();
			Emit("ceq");
		}
		else if (node->op == "<=")
		{
			Emit("cgt");
			LdcI4_False();
			Emit("ceq");
		}
		else if (node->op == ">=")
		{
			Emit("clt");
			LdcI4_False();
			Emit("ceq");
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
		BrFalse(node->elseBranch ? elseLabel : endLabel);

		if (node->thenBranch)
		{
			node->thenBranch->Accept(*this);
		}

		if (node->elseBranch)
		{
			Br(endLabel);
			MarkLabel(elseLabel);
			node->elseBranch->Accept(*this);
		}
		MarkLabel(endLabel);
	}

	void Visit(const ast::WhileStmt* node) override
	{
		const std::string startLabel = "L_while_start_" + std::to_string(m_labelCount);
		const std::string endLabel = "L_while_end_" + std::to_string(m_labelCount++);

		MarkLabel(startLabel);
		if (node->condition)
		{
			node->condition->Accept(*this);
		}
		BrFalse(endLabel);

		if (node->body)
		{
			node->body->Accept(*this);
		}

		Br(startLabel);
		MarkLabel(endLabel);
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

			LdcI8(0);
			StLoc(idxIdx);

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

			MarkLabel(startLabel);

			LdLoc(idxIdx);
			LdLoc(arrIdx);
			Emit("ldlen");
			Emit("conv.i8");
			Emit("bge " + endLabel);

			LdLoc(arrIdx);
			LdLoc(idxIdx);
			Emit("conv.i4");
			Emit("ldelem." + GetElemSuffix(pureSemType));
			StLoc(iterIdx);

			if (node->body)
			{
				node->body->Accept(*this);
			}

			LdLoc(idxIdx);
			LdcI8(1);
			Emit("add");
			StLoc(idxIdx);

			Br(startLabel);
			MarkLabel(endLabel);
			return;
		}

		const std::string startLabel = "L_for_start_" + std::to_string(m_labelCount);
		const std::string endLabel = "L_for_end_" + std::to_string(m_labelCount++);

		DeclareLocal(node->iteratorName, node->startExpr.get());
		const int iterIdx = GetLocalIndex(node->iteratorName);

		const std::string limitName = "_for_limit_" + std::to_string(m_labelCount);
		DeclareLocal(limitName, node->endExpr.get());
		const int limitIdx = GetLocalIndex(limitName);

		MarkLabel(startLabel);
		LdLoc(iterIdx);
		LdLoc(limitIdx);
		Emit("bgt " + endLabel);

		if (node->body)
		{
			node->body->Accept(*this);
		}

		LdLoc(iterIdx);
		LdcI8(1);
		Emit("add");
		StLoc(iterIdx);

		Br(startLabel);
		MarkLabel(endLabel);
	}

	void Visit(const ast::CallExpr* node) override
	{
		if (!m_semanticAnalyzer.GetBindings().callInfo.contains(node))
		{
			auto calleeType = m_semanticAnalyzer.GetBindings().GetExpressionType(node->callee.get());
			if (auto funType = std::dynamic_pointer_cast<sem::FunctionType>(calleeType))
			{
				node->callee->Accept(*this);

				for (const auto& arg : node->arguments)
				{
					if (arg)
					{
						arg->Accept(*this);
					}
				}

				const re::String delegateName = GetDelegateName(funType.get());
				re::String invokeSig = BuildCilSignature(funType.get(), delegateName, "Invoke", false, false);

				CallVirtual(invokeSig);
				return;
			}
		}

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
				Emit("ldc.i4 " + std::to_string(varargCount));

				const re::String elemType = callInfo.target->paramTypes.back()->name;
				re::String cilElemType = MapToCIL(elemType);
				if (cilElemType.Find("[]") != re::String::NPos)
				{
					cilElemType = cilElemType.Substring(0, cilElemType.Length() - 2);
				}

				Emit("newarr " + cilElemType);

				for (std::size_t i = 0; i < varargCount; ++i)
				{
					Emit("dup");
					Emit("ldc.i4 " + std::to_string(i));

					if (node->arguments[normalCount + i])
					{
						node->arguments[normalCount + i]->Accept(*this);

						if (elemType == "Any" || elemType == "System.Object")
						{
							if (const auto argSemType = m_semanticAnalyzer.GetBindings().GetExpressionType(node->arguments[normalCount + i].get()))
							{
								if (argSemType->name == "System.Int64")
								{
									Emit("box [mscorlib]System.Int64");
								}
								else if (argSemType->name == "System.Double")
								{
									Emit("box [mscorlib]System.Double");
								}
								else if (argSemType->name == "System.Boolean")
								{
									Emit("box [mscorlib]System.Boolean");
								}
							}
						}
					}
					Emit("stelem." + GetElemSuffix(elemType));
				}
			}
			else
			{
				for (const auto& arg : node->arguments)
				{
					if (arg)
					{
						arg->Accept(*this);
					}
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
				Emit("ldlen");
				Emit("conv.i8");
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
				Emit("conv.i4");

				re::String elemType = "System.Object";
				if (const auto classType = std::dynamic_pointer_cast<sem::ClassType>(callInfo.target->paramTypes[0]))
				{
					if (!classType->typeArguments.empty())
					{
						elemType = classType->typeArguments[0]->name;
					}
				}
				Emit("ldelem." + GetElemSuffix(elemType));
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
				Emit("conv.i4");
				if (node->arguments.size() > 1 && node->arguments[1])
				{
					node->arguments[1]->Accept(*this);
				}

				re::String elemType = "System.Object";
				if (const auto classType = std::dynamic_pointer_cast<sem::ClassType>(callInfo.target->paramTypes[0]))
				{
					if (!classType->typeArguments.empty())
						elemType = classType->typeArguments[0]->name;
				}
				Emit("stelem." + GetElemSuffix(elemType));
			}
			else if (op == "newarr")
			{
				emitArgs(true);
				re::String elemType = "class [mscorlib]System.Object";
				if (const auto classType = std::dynamic_pointer_cast<sem::ClassType>(callInfo.target->returnType))
				{
					if (!classType->typeArguments.empty())
						elemType = MapToCIL(classType->typeArguments[0]->name);
				}
				Emit("newarr " + elemType);
			}
			else
			{
				emitArgs();
				Emit(op);
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
			const re::String invokeSig = BuildCilSignature(callInfo.target.get(), callInfo.target->name, "Invoke", false, false);
			CallVirtual(invokeSig);
			return;
		}

		if (callInfo.dispatchMode == CallDispatchType::Native)
		{
			if (const auto dotnetMethod = ast::AnnotationUtils::GetAnnotationArg(targetAnnos, "DotNetMethod"))
			{
				emitArgs(true);
				Call(*dotnetMethod);
				return;
			}
			emitArgs();
			Call(callInfo.target->name);
			return;
		}

		if (callInfo.isConstructorCall)
		{
			emitArgs();
			Emit("newobj instance void " + callInfo.mangledClassName + "::.ctor(" + BuildTypeSignature(callInfo.target->paramTypes, 1, callInfo.target->isVararg) + ")");
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
				LdArg(0);
			}

			emitArgs();
			const re::String virtSig = BuildCilSignature(callInfo.target.get(), callInfo.target->paramTypes[0]->name, callInfo.asmLabel, false, true);
			CallVirtual(virtSig);
			return;
		}

		re::String finalAsmLabel = callInfo.asmLabel;
		re::String cleanName = finalAsmLabel;

		if (const std::size_t firstAt = cleanName.Find('@'); firstAt != re::String::NPos)
		{
			cleanName = cleanName.Substring(0, firstAt);
		}

		if (cleanName.Length() > 0)
		{
			if (const std::size_t underPos = cleanName.Find('_'); underPos != re::String::NPos)
			{
				re::String prefix = cleanName.Substring(0, underPos);
				re::String suffix = cleanName.Substring(underPos + 1, cleanName.Length() - underPos - 1);

				if (prefix == suffix && m_currentClass && callInfo.dispatchMode != CallDispatchType::Virtual)
				{
					LdArg(0);
					emitArgs();
					Emit("call instance void " + prefix + "::.ctor(" + BuildTypeSignature(callInfo.target->paramTypes, 1, callInfo.target->isVararg) + ")");
					return;
				}
			}
		}

		if (const auto memAccess = dynamic_cast<const ast::MemberAccessExpr*>(node->callee.get()))
		{
			memAccess->object->Accept(*this);
		}
		else if (callInfo.isImplicitThisCall)
		{
			LdArg(0);
		}

		emitArgs();
		const re::String globalSig = BuildCilSignature(callInfo.target.get(), "IgniGlobalModule", finalAsmLabel, false, false);
		Call(globalSig);
	}

	void Visit(const ast::LambdaExpr* node) override
	{
		const auto semType = m_semanticAnalyzer.GetBindings().GetExpressionType(node);
		const auto funType = std::dynamic_pointer_cast<sem::FunctionType>(semType);
		const re::String lambdaClassName = semType->name;

		std::vector<re::String> capTypes;
		std::vector<bool> isByRef;

		for (const auto& capName : node->captures)
		{
			re::String cilType = "class [mscorlib]System.Object";
			bool refFlag = false;

			if (const auto* currentLambda = m_lambdaHelper.GetCurrentLambda())
			{
				for (std::size_t i = 0; i < currentLambda->node->captures.size(); ++i)
				{
					if (currentLambda->node->captures[i] == capName)
					{
						cilType = currentLambda->captureTypes[i];
						LdArg(0);
						LdFld(cilType + (currentLambda->isByRef[i] ? "[]" : ""), currentLambda->className, capName);

						refFlag = currentLambda->isByRef[i];
						break;
					}
				}
			}

			if (!refFlag)
			{
				if (m_globalVars.contains(capName))
				{
					cilType = m_globalVars.at(capName);
					Emit("ldsfld " + cilType + " IgniGlobalModule::" + capName);
				}
				else if (const int locIdx = GetLocalIndex(capName); locIdx != -1)
				{
					cilType = m_localTypes[locIdx];
					if (cilType.Find("[]") != re::String::NPos)
					{
						cilType = cilType.Substring(0, cilType.Length() - 2);
						refFlag = true;
					}
					LdLoc(locIdx);
				}
				else if (const int argIdx = GetArgIndex(capName); argIdx != -1)
				{
					cilType = m_argTypes[argIdx];
					LdArg(argIdx);
				}
			}
			capTypes.push_back(cilType);
			isByRef.push_back(refFlag);
		}

		re::String sig = "instance void " + lambdaClassName + "::.ctor(";
		for (std::size_t i = 0; i < capTypes.size(); ++i)
		{
			sig += capTypes[i] + (isByRef[i] ? "[]" : "") + (i < capTypes.size() - 1 ? ", " : "");
		}
		sig += ")";

		Emit("newobj " + sig);
		m_lambdaHelper.RegisterLambda({ node, lambdaClassName, capTypes, isByRef });
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
						Pop();
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
		Ret();
	}

	void Visit(const ast::IndexExpr* node) override
	{
		node->array->Accept(*this);
		node->index->Accept(*this);
		Emit("conv.i4");

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
		Emit("ldelem." + GetElemSuffix(elemType));
	}

	void Visit(const ast::TypeCastExpr* node) override
	{
		node->expr->Accept(*this);

		if (m_semanticAnalyzer.GetBindings().castTargets.contains(node))
		{
			Emit("castclass " + MapToCIL(m_semanticAnalyzer.GetBindings().castTargets.at(node)));
		}
	}

private:
	std::ostream& m_out;
	std::ostream* m_currentOut = &m_out;
	std::stringstream m_methodBuffer;
	const sem::SemanticAnalyzer& m_semanticAnalyzer;

	bool m_isWritingGlobal = true;

	const ast::ClassDecl* m_currentClass = nullptr;
	std::vector<re::String> m_locals;
	std::vector<re::String> m_localTypes;
	std::vector<re::String> m_args;
	std::size_t m_labelCount = 0;

	std::vector<re::String> m_externAssemblies;
	std::unordered_map<re::String, re::String> m_globalVars;

	std::vector<re::String> m_argTypes;

	dotnet::LambdaHelper m_lambdaHelper;

	void SetOutStream(std::ostream& out)
	{
		m_currentOut = &out;
		SetStream(out);
	}

	void PrepareMethodScope(const bool hasThis, const std::vector<std::unique_ptr<ast::ParameterNode>>& params, const ast::Block* body = nullptr)
	{
		m_isWritingGlobal = false;
		m_locals.clear();
		m_localTypes.clear();
		m_args.clear();
		m_argTypes.clear();

		if (body)
		{
			m_lambdaHelper.ScanCaptures(body);
		}

		if (hasThis)
		{
			m_args.emplace_back("this");
			m_argTypes.emplace_back(m_currentClass ? "class " + m_currentClass->name : "class [mscorlib]System.Object");
		}
		for (const auto& param : params)
		{
			m_args.push_back(param->name);
			m_argTypes.push_back(MapAstType(param->type.get()));
		}

		m_methodBuffer.str("");
		m_methodBuffer.clear();

		SetOutStream(m_methodBuffer);
	}

	void EmitMethodBody(const re::String& signature, const ast::Block* body, const bool isEntryPoint)
	{
		if (body)
		{
			body->Accept(*this);
		}

		SetOutStream(m_out);

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
		m_isWritingGlobal = true;
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
		if (const auto* lambdaData = m_lambdaHelper.GetCurrentLambda())
		{
			for (size_t i = 0; i < lambdaData->node->captures.size(); ++i)
			{
				if (lambdaData->node->captures[i] == id->name)
				{
					const re::String cilType = lambdaData->captureTypes[i];
					const bool isByRef = lambdaData->isByRef[i];

					LdArg(0);
					LdFld(cilType + (isByRef ? "[]" : ""), lambdaData->className, id->name);

					if (isByRef)
					{
						if (isStore)
						{
							LdcI4(0);
							if (assignValue)
							{
								assignValue->Accept(*this);
							}
							StElem(GetElemSuffix(cilType));
						}
						else
						{
							LdcI4(0);
							LdElem(GetElemSuffix(cilType));
						}
					}
					else
					{
						if (isStore)
						{
							if (assignValue)
							{
								assignValue->Accept(*this);
							}
							LdArg(0);
							StFld(cilType, lambdaData->className, id->name);
						}
					}
					return;
				}
			}
		}

		if (m_semanticAnalyzer.GetBindings().implicitThisNames.contains(id))
		{
			LdArg(0); // 'this'
			if (assignValue)
			{
				assignValue->Accept(*this);
			}

			const auto classType = m_semanticAnalyzer.GetClassType(m_currentClass->name);
			const auto fieldType = classType->fields.at(id->name).type;

			if (isStore)
			{
				StFld(MapToCIL(fieldType->name), classType->name, id->name);
			}
			else
			{
				LdFld(MapToCIL(fieldType->name), classType->name, id->name);
			}

			return;
		}

		if (assignValue)
		{
			assignValue->Accept(*this);
		}

		if (m_globalVars.contains(id->name))
		{
			if (isStore)
			{
				StSFld(m_globalVars.at(id->name), "IgniGlobalModule", id->name);
			}
			else
			{
				LdSFld(m_globalVars.at(id->name), "IgniGlobalModule", id->name);
			}
			return;
		}

		if (const int locIdx = GetLocalIndex(id->name); locIdx != -1)
		{
			const bool isCaptured = m_lambdaHelper.IsCaptured(id->name);
			if (isCaptured)
			{
				LdLoc(locIdx);
				if (isStore)
				{
					LdcI4(0);
					if (assignValue)
					{
						assignValue->Accept(*this);
					}
					const re::String baseType = m_localTypes[locIdx].Substring(0, m_localTypes[locIdx].Length() - 2);
					StElem(GetElemSuffix(baseType));
				}
				else
				{
					LdcI4(0);
					const re::String baseType = m_localTypes[locIdx].Substring(0, m_localTypes[locIdx].Length() - 2);
					LdElem(GetElemSuffix(baseType));
				}
			}
			else
			{
				if (isStore)
				{
					StLoc(locIdx);
				}
				else
				{
					LdLoc(locIdx);
				}
			}

			return;
		}

		if (const int argIdx = GetArgIndex(id->name); argIdx != -1)
		{
			if (isStore)
			{
				Emit("starg " + std::to_string(argIdx));
			}
			else
			{
				LdArg(argIdx);
			}
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

		const bool isCaptured = m_lambdaHelper.IsCaptured(name);
		m_locals.push_back(name);
		m_localTypes.push_back(cilType + (isCaptured ? "[]" : ""));

		const int idx = static_cast<int>(m_locals.size() - 1);

		if (isCaptured)
		{
			LdcI4(1);
			NewArr(cilType);
			StLoc(idx);

			LdLoc(idx);
			LdcI4(0);
		}

		if (initExpr)
		{
			initExpr->Accept(*this);
		}
		else
		{
			LdcI8(0);
		}

		if (isCaptured)
		{
			StElem(GetElemSuffix(cilType));
		}
		else
		{
			StLoc(idx);
		}
	}

	[[nodiscard]] static re::String BuildCilSignature(
		const sem::FunctionType* funType,
		const re::String& ownerClass,
		const re::String& methodName,
		const bool isInstance = false,
		const bool skipFirstParam = false)
	{
		const re::String retCilType = funType->returnType ? MapToCIL(funType->returnType->name) : TYPE_VOID;
		const re::String prefix = ownerClass.Empty() ? re::String("") : ownerClass + "::";
		re::String sig = (isInstance ? "instance " : "") + retCilType + " " + prefix + methodName + "(";
		sig += BuildTypeSignature(funType->paramTypes, skipFirstParam ? 1 : 0, funType->isVararg);
		sig += ")";

		return sig;
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

	[[nodiscard]] static re::String GetBaseClass(const std::vector<std::unique_ptr<ast::AnnotationNode>>& annotations)
	{
		for (const auto& anno : annotations)
		{
			if (anno->name == ANNO_BASE_CLASS)
			{
				if (const auto strArg = ast::AnnotationUtils::GetAnnotationStringArg(anno.get()))
				{
					return *strArg;
				}
			}
		}
		return "[mscorlib]System.Object";
	}

	[[nodiscard]] static re::String GetDelegateName(const sem::FunctionType* funType)
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
		sigName += "_Ret_" + rName;

		return sigName;
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

	void RegisterDelegateFromType(const std::shared_ptr<sem::SemanticType>& type)
	{
		if (const auto funType = std::dynamic_pointer_cast<sem::FunctionType>(type))
		{
			dotnet::LambdaHelper::DelegateInfo info;
			info.className = GetDelegateName(funType.get());
			info.returnType = funType->returnType ? MapToCIL(funType->returnType->name) : TYPE_VOID;

			for (const auto& pt : funType->paramTypes)
			{
				info.paramTypes.push_back(MapToCIL(pt->name));
			}
			m_lambdaHelper.RegisterDelegate(info);
		}
	}

	template <typename Container, typename Value>
	[[nodiscard]] static int IndexOf(const Container& container, const Value& value)
	{
		const auto it = std::ranges::find(container, value);

		return it != container.end() ? static_cast<int>(std::distance(container.begin(), it)) : -1;
	}

	[[nodiscard]] int GetLocalIndex(const re::String& name) const
	{
		return IndexOf(m_locals, name);
	}

	[[nodiscard]] int GetArgIndex(const re::String& name) const
	{
		return IndexOf(m_args, name);
	}
};

} // namespace igni