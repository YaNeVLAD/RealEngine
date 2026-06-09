#pragma once

#include <Core/flat_map.hpp>
#include <IgniLang/AST/AstNodes.hpp>
#include <IgniLang/AST/Utils/AnnotationUtils.hpp>
#include <IgniLang/Compiler/DotNet/CILEmitter.hpp>
#include <IgniLang/Compiler/DotNet/CallProcessor.hpp>
#include <IgniLang/Compiler/DotNet/ControlFlowProcessor.hpp>
#include <IgniLang/Compiler/DotNet/GeneratorState.hpp>
#include <IgniLang/Compiler/DotNet/GlobalProcessor.hpp>
#include <IgniLang/Compiler/DotNet/LambdaHelper.hpp>
#include <IgniLang/Compiler/DotNet/SignatureUtils.hpp>
#include <IgniLang/Compiler/DotNet/TypeMapper.hpp>
#include <IgniLang/Semantic/Helpers/NameMangler.hpp>
#include <IgniLang/Semantic/SemanticAnalyzer.hpp>

#include <optional>
#include <ostream>
#include <sstream>
#include <vector>

namespace igni
{

class DotNetCodeGenerator final
	: public ast::BaseAstVisitor
	, public dotnet::TypeMapper
	, public dotnet::CILEmitter
	, dotnet::GlobalProcessor
	, dotnet::ControlFlowProcessor
	, dotnet::CallProcessor
{
	template <std::size_t N>
	using HashedStringMap = re::flat_map<re::HashedString, std::string_view, N>;

	static constexpr auto TYPE_OBJECT = "class [mscorlib]System.Object";
	static constexpr auto TYPE_VOID = "void";
	static constexpr auto ANNO_BASE_CLASS = "DotNetBaseClass";

public:
	std::stringstream m_methodBuffer;
	std::ostream* m_currentOut = nullptr;

	dotnet::detail::GeneratorState m_state;
	dotnet::LambdaHelper m_lambdaHelper;

public:
	DotNetCodeGenerator(std::ostream& out, const sem::SemanticAnalyzer& semantics)
		: CILEmitter(out)
		, GlobalProcessor(*this)
		, ControlFlowProcessor(*this)
		, CallProcessor(*this)
		, m_state(out, semantics)
		, m_lambdaHelper(*this, semantics)
	{
		m_currentOut = &m_state.out;
	}

	void Generate(const ast::Program* program)
	{
		ProcessAssemblies(program);

		m_state.out << "// Auto-generated .NET CIL\n.assembly IgniProgram { }\n.assembly extern mscorlib { }\n\n";
		for (const auto& asmName : m_state.externAssemblies)
		{
			m_state.out << ".assembly extern " << asmName << " { }\n";
		}
		m_state.out << "\n.class public auto ansi beforefieldinit IgniGlobalModule extends [mscorlib]System.Object\n{\n";

		ProcessGlobalFields(program);
		ProcessGlobalConstructor(program);

		SetOutStream(m_state.out);
		for (const auto& stmt : program->statements)
		{
			if (!dynamic_cast<const ast::ClassDecl*>(stmt.get()) && stmt)
			{
				stmt->Accept(*this);
			}
		}
		m_state.out << "}\n\n";

		for (const auto& stmt : program->statements)
		{
			if (const auto classDecl = dynamic_cast<const ast::ClassDecl*>(stmt.get()))
			{
				classDecl->Accept(*this);
			}
		}

		m_lambdaHelper.GenerateDelegates(m_state.out);
		m_lambdaHelper.GenerateAllClasses(m_state.out, [&](const re::String& sig, const ast::Block* body, const bool isEntry, const auto& params) {
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

		m_state.currentClass = node;
		const re::String baseClass = GetBaseClass(node->annotations);

		m_state.out << ".class public auto ansi beforefieldinit " << node->name << " extends " << baseClass << "\n{\n";

		if (const auto semClass = m_state.analyzer.GetClassType(node->name))
		{
			for (const auto& [fieldName, fieldInfo] : semClass->fields)
			{
				m_state.out << "  .field public " << MapToCIL(fieldInfo.type->name) << " '" << fieldName << "'\n";
			}
		}

		for (const auto& member : node->members)
		{
			if (dynamic_cast<const ast::FunDecl*>(member.get()) || dynamic_cast<const ast::ConstructorDecl*>(member.get()))
			{
				member->Accept(*this);
			}
		}

		m_state.out << "}\n\n";
		m_state.currentClass = nullptr;
	}

	void Visit(const ast::ConstructorDecl* node) override
	{
		if (node->isExternal)
		{
			return;
		}

		PrepareMethodScope(true, node->parameters, node->body.get());

		if (const re::String baseClass = GetBaseClass(m_state.currentClass->annotations); baseClass == "[mscorlib]System.Object")
		{
			LdArg0();
			Call("instance void [mscorlib]System.Object::.ctor()");
		}

		for (const auto& member : m_state.currentClass->members)
		{
			if (const auto varDecl = dynamic_cast<const ast::VarDecl*>(member.get()))
			{
				if (varDecl->initializer)
				{
					LdArg0();
					varDecl->initializer->Accept(*this);
					const auto semType = m_state.analyzer.GetBindings().GetExpressionType(varDecl->initializer.get());
					StFld(MapToCIL(semType->name), m_state.currentClass->name, varDecl->name);
				}
			}
			else if (const auto valDecl = dynamic_cast<const ast::ValDecl*>(member.get()))
			{
				if (valDecl->initializer)
				{
					LdArg0();
					valDecl->initializer->Accept(*this);
					const auto semType = m_state.analyzer.GetBindings().GetExpressionType(valDecl->initializer.get());
					StFld(MapToCIL(semType->name), m_state.currentClass->name, valDecl->name);
				}
			}
		}

		re::String sig = ".method public hidebysig specialname rtspecialname instance void .ctor(";
		sig += dotnet::SignatureUtils::BuildTypeSignature(node->parameters, 0, false);
		sig += ") cil managed";
		EmitMethodBody(sig, node->body.get(), false);
	}

	void Visit(const ast::FunDecl* node) override
	{
		if (node->isExternal || !node->typeParams.empty())
		{
			return;
		}

		const std::shared_ptr<sem::FunctionType> funType = LookupFunctionType(node);
		const bool isInstanceMethod = m_state.currentClass != nullptr;
		const bool hasImplicitThis = funType && funType->paramTypes.size() > node->parameters.size() && !isInstanceMethod;

		PrepareMethodScope(isInstanceMethod, node->parameters, node->body.get());

		if (hasImplicitThis)
		{
			m_state.args.insert(m_state.args.begin(), "this");
			m_state.argTypes.insert(m_state.argTypes.begin(), MapToCIL(funType->paramTypes[0]->name));
		}

		bool hasDllExport = false;
		for (const auto& anno : node->annotations)
		{
			if (anno->name == "DllExport")
			{
				hasDllExport = true;
				break;
			}
		}

		re::String methodName = node->name;
		if (!isInstanceMethod && node->name != "main" && !hasDllExport)
		{
			methodName = m_state.analyzer.GetBindings().GetMangledName(node);
		}

		if (m_state.currentClass)
		{
			methodName = node->name;
		}

		re::String sig;
		if (funType)
		{
			sig = dotnet::SignatureUtils::BuildCilSignature(funType.get(), "", methodName, isInstanceMethod, isInstanceMethod);
			if (!isInstanceMethod)
			{
				re::String staticSig = "static ";
				staticSig += sig;
				sig = staticSig;
			}
		}
		else
		{
			const re::String retType = node->returnType ? MapAstType(node->returnType.get()) : TYPE_VOID;
			re::String fallbackSig = isInstanceMethod ? "instance " : "static ";
			fallbackSig += retType;
			fallbackSig += " ";
			fallbackSig += methodName;
			fallbackSig += "(";
			fallbackSig += dotnet::SignatureUtils::BuildTypeSignature(node->parameters, 0, node->isVararg);
			fallbackSig += ")";
			sig = fallbackSig;
		}

		re::String finalSig = ".method public hidebysig ";
		finalSig += sig;
		finalSig += " cil managed";
		EmitMethodBody(finalSig, node->body.get(), node->name == "main" && !m_state.currentClass);
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
		if (m_state.isWritingGlobal)
		{
			return;
		}
		DeclareLocal(node->name, node->initializer.get());
	}

	void Visit(const ast::ValDecl* node) override
	{
		if (m_state.isWritingGlobal)
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
			ConvI4();
			if (node->value)
			{
				node->value->Accept(*this);
			}

			re::String elemType = "System.Object";
			if (const auto arrSemType = m_state.analyzer.GetBindings().GetExpressionType(idxAccess->array.get()))
			{
				if (const auto classType = std::dynamic_pointer_cast<sem::ClassType>(arrSemType))
				{
					if (!classType->typeArguments.empty())
					{
						elemType = classType->typeArguments[0]->name;
					}
				}
			}
			StElem(GetElemSuffix(elemType));
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
		const auto exprType = m_state.analyzer.GetBindings().GetExpressionType(node);

		if (const auto funType = std::dynamic_pointer_cast<sem::FunctionType>(exprType))
		{
			if (!m_state.globalVars.contains(node->name) && m_state.GetLocalIndex(node->name) == -1 && m_state.GetArgIndex(node->name) == -1)
			{
				RegisterDelegateFromType(funType);
				const re::String delegateName = dotnet::SignatureUtils::GetDelegateName(funType.get());

				std::vector<re::String> typeNames;
				for (const auto& pt : funType->paramTypes)
				{
					typeNames.push_back(pt->name);
				}

				const re::String mangledName = sem::NameMangler::Mangle(node->name, typeNames, false);
				const re::String targetMethodSig = dotnet::SignatureUtils::BuildCilSignature(funType.get(), "IgniGlobalModule", mangledName, false, false);

				LdNull();
				re::String ldftnInstr = "ldftn ";
				ldftnInstr += targetMethodSig;
				Emit(ldftnInstr);

				re::String newObjInstr = delegateName;
				newObjInstr += "::.ctor(object, native int)";
				NewObj(newObjInstr);
				return;
			}
		}

		EmitIdentifierAccess(node, false, nullptr);
	}

	void Visit(const ast::LiteralExpr* node) override
	{
		switch (node->token.type)
		{
		case TokenType::IntConst:
			LdcI8(std::stoll(std::string(node->token.lexeme)));
			break;
		case TokenType::FloatConst:
			Emit(re::String("ldc.r8 ") + node->token.lexeme);
			break;
		case TokenType::StringConst:
			Emit(re::String("ldstr ") + node->token.lexeme);
			break;
		case TokenType::KwNull:
			Emit("ldnull");
			break;
		case TokenType::KwTrue:
			LdcI4_True();
			break;
		case TokenType::KwFalse:
			LdcI4_False();
			break;
		default:
			break;
		}
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
				Dup();
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
		ProcessIfStmt(node);
	}

	void Visit(const ast::WhileStmt* node) override
	{
		ProcessWhileStmt(node);
	}

	void Visit(const ast::ForStmt* node) override
	{
		ProcessForStmt(node);
	}

	void Visit(const ast::CallExpr* node) override
	{
		if (m_state.analyzer.GetBindings().callInfo.contains(node))
		{
			if (const auto& callInfo = m_state.analyzer.GetBindings().callInfo.at(node);
				callInfo.dispatchMode == CallDispatchType::Indirect)
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

				const re::String retType = callInfo.target && callInfo.target->returnType
					? MapToCIL(callInfo.target->returnType->name)
					: re::String("void");

				re::String invokeSig = "instance " + retType + " ";

				if (const auto funType = std::dynamic_pointer_cast<sem::FunctionType>(callInfo.target))
				{
					invokeSig += "class " + dotnet::SignatureUtils::GetDelegateName(funType.get());
				}
				else
				{
					invokeSig += callInfo.target->name;
				}

				invokeSig += "::Invoke(";
				for (size_t i = 0; i < callInfo.target->paramTypes.size(); ++i)
				{
					re::String mapped = MapToCIL(callInfo.target->paramTypes[i]->name);
					if (callInfo.target->isVararg && i == callInfo.target->paramTypes.size() - 1)
					{
						if (mapped.Find("[]") == re::String::NPos)
						{
							mapped += "[]";
						}
					}
					invokeSig += mapped;
					if (i < callInfo.target->paramTypes.size() - 1)
					{
						invokeSig += ", ";
					}
				}
				invokeSig += ")";

				CallVirtual(invokeSig);
				return;
			}
		}

		ProcessCallExpr(node);
	}

	void Visit(const ast::LambdaExpr* node) override
	{
		const auto semType = m_state.analyzer.GetBindings().GetExpressionType(node);
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
						re::String fieldSig = cilType;
						if (currentLambda->isByRef[i])
						{
							fieldSig += "[]";
						}
						LdFld(fieldSig, currentLambda->className, capName);

						refFlag = currentLambda->isByRef[i];
						break;
					}
				}
			}

			if (!refFlag)
			{
				if (m_state.globalVars.contains(capName))
				{
					cilType = m_state.globalVars.at(capName);
					LdSFld(cilType, "IgniGlobalModule", capName);
				}
				else if (const int locIdx = m_state.GetLocalIndex(capName); locIdx != -1)
				{
					cilType = m_state.localTypes[locIdx];
					if (cilType.Find("[]") != re::String::NPos)
					{
						cilType = cilType.Substring(0, cilType.Length() - 2);
						refFlag = true;
					}
					LdLoc(locIdx);
				}
				else if (const int argIdx = m_state.GetArgIndex(capName); argIdx != -1)
				{
					cilType = m_state.argTypes[argIdx];
					LdArg(argIdx);
				}
			}
			capTypes.push_back(cilType);
			isByRef.push_back(refFlag);
		}

		re::String sig = lambdaClassName + "::.ctor(";
		for (std::size_t i = 0; i < capTypes.size(); ++i)
		{
			sig += capTypes[i];
			if (isByRef[i])
			{
				sig += "[]";
			}
			if (i < capTypes.size() - 1)
			{
				sig += ", ";
			}
		}
		sig += ")";

		NewObj(sig);
		m_lambdaHelper.RegisterLambda({ node, lambdaClassName, capTypes, isByRef });

		if (funType)
		{
			RegisterDelegateFromType(funType);
			const re::String delegateName = dotnet::SignatureUtils::GetDelegateName(funType.get());
			const re::String invokeSig = dotnet::SignatureUtils::BuildCilSignature(funType.get(), lambdaClassName, "Invoke", true, false);

			re::String ldftnInstr = "ldftn ";
			ldftnInstr += invokeSig;
			Emit(ldftnInstr);

			re::String newObjInstr = delegateName;
			newObjInstr += "::.ctor(object, native int)";
			NewObj(newObjInstr);
		}
	}

	void Visit(const ast::ExprStmt* node) override
	{
		if (node->expr)
		{
			node->expr->Accept(*this);

			if (!dynamic_cast<const ast::AssignExpr*>(node->expr.get()))
			{
				if (const auto semType = m_state.analyzer.GetBindings().GetExpressionType(node->expr.get()))
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

	void Visit(const ast::TypeCastExpr* node) override
	{
		node->expr->Accept(*this);

		if (m_state.analyzer.GetBindings().castTargets.contains(node))
		{
			CastClass(MapToCIL(m_state.analyzer.GetBindings().castTargets.at(node)));
		}
	}

	void SetOutStream(std::ostream& out)
	{
		m_currentOut = &out;
		SetStream(out);
	}

	void DeclareLocal(const re::String& name, const ast::Expr* initExpr)
	{
		re::String cilType = "int64";
		if (initExpr)
		{
			if (const auto semType = m_state.analyzer.GetBindings().GetExpressionType(initExpr))
			{
				if (const auto funType = std::dynamic_pointer_cast<sem::FunctionType>(semType))
				{
					RegisterDelegateFromType(funType);
					cilType = "class " + dotnet::SignatureUtils::GetDelegateName(funType.get());
				}
				else
				{
					cilType = MapToCIL(semType->name);
				}
			}
		}

		const bool isCaptured = m_lambdaHelper.IsCaptured(name);
		m_state.locals.push_back(name);

		re::String localTypeStr = cilType;
		if (isCaptured)
		{
			localTypeStr += "[]";
		}
		m_state.localTypes.push_back(localTypeStr);

		const int idx = static_cast<int>(m_state.locals.size() - 1);

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

private:
	void PrepareMethodScope(const bool hasThis, const std::vector<std::unique_ptr<ast::ParameterNode>>& params, const ast::Block* body = nullptr)
	{
		m_state.isWritingGlobal = false;
		m_state.locals.clear();
		m_state.localTypes.clear();
		m_state.args.clear();
		m_state.argTypes.clear();

		if (body)
		{
			m_lambdaHelper.ScanCaptures(body);
		}

		if (hasThis)
		{
			m_state.args.emplace_back("this");
			m_state.argTypes.emplace_back(m_state.currentClass ? re::String("class ") + m_state.currentClass->name : re::String("class [mscorlib]System.Object"));
		}
		for (const auto& param : params)
		{
			m_state.args.push_back(param->name);
			m_state.argTypes.push_back(MapAstType(param->type.get()));
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

		SetOutStream(m_state.out);

		m_state.out << "  " << signature << "\n  {\n";
		if (isEntryPoint)
		{
			m_state.out << "    .entrypoint\n";
		}
		m_state.out << "    .maxstack 8\n";

		if (!m_state.locals.empty())
		{
			m_state.out << "    .locals init (\n";
			for (size_t i = 0; i < m_state.locals.size(); ++i)
			{
				m_state.out << "      [" << i << "] " << m_state.localTypes[i] << " '" << m_state.locals[i] << "'" << (i == m_state.locals.size() - 1 ? "" : ",") << "\n";
			}
			m_state.out << "    )\n";
		}

		m_state.out << m_methodBuffer.str() << "    ret\n  }\n\n";
		m_state.isWritingGlobal = true;
	}

	std::shared_ptr<sem::FunctionType> LookupFunctionType(const ast::FunDecl* node) const
	{
		if (m_state.currentClass)
		{
			if (const auto semClass = m_state.analyzer.GetClassType(m_state.currentClass->name))
			{
				if (semClass->methods.contains(node->name))
				{
					if (const auto fg = std::dynamic_pointer_cast<sem::FunctionGroup>(semClass->methods.at(node->name)))
					{
						for (const auto& overload : fg->overloads)
						{
							if (overload->paramTypes.size() == node->parameters.size() + 1)
							{
								return overload;
							}
						}
					}
					return std::dynamic_pointer_cast<sem::FunctionType>(semClass->methods.at(node->name));
				}
			}
		}
		else
		{
			auto environment = m_state.analyzer.Env();
			if (const auto* sym = environment.Resolve(node->name))
			{
				if (const auto fg = std::dynamic_pointer_cast<sem::FunctionGroup>(sym->type))
				{
					for (const auto& overload : fg->overloads)
					{
						if (overload->paramTypes.size() == node->parameters.size())
						{
							return overload;
						}
					}
				}
				return std::dynamic_pointer_cast<sem::FunctionType>(sym->type);
			}

			const re::String targetMangled = m_state.analyzer.GetBindings().GetMangledName(node);
			for (const auto& info : m_state.analyzer.GetBindings().callInfo | std::views::values)
			{
				if (info.asmLabel == targetMangled)
				{
					return info.target;
				}
			}
		}
		return nullptr;
	}

	std::shared_ptr<sem::ClassType> ResolveClassType(const ast::Expr* objExpr) const
	{
		const auto objSemType = m_state.analyzer.GetBindings().GetExpressionType(objExpr);
		if (!objSemType)
		{
			if (const auto id = dynamic_cast<const ast::IdentifierExpr*>(objExpr); id && id->name == "this")
			{
				return m_state.analyzer.GetClassType(m_state.currentClass->name);
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
				if (isStore)
				{
					StFld(MapToCIL(fieldType->name), classType->name, member);
				}
				else
				{
					LdFld(MapToCIL(fieldType->name), classType->name, member);
				}
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

					LdArg(0); // 'this'

					re::String fieldSig = cilType;
					if (isByRef)
					{
						fieldSig += "[]";
					}

					if (isByRef)
					{
						LdFld(fieldSig, lambdaData->className, id->name);
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
							// Для записи в обычную захваченную переменную (копия)
							// assignValue уже на стеке, нам нужен 'this' перед stfld.
							// Но так как stfld требует [obj, value], а у нас на стеке [value],
							// архитектурно проще было сделать ldarg.0 до вычисления assignValue.
							// Перепишем по спецификации CLR:
						}
						else
						{
							LdFld(fieldSig, lambdaData->className, id->name);
						}
					}
					return;
				}
			}
		}

		if (m_state.analyzer.GetBindings().implicitThisNames.contains(id))
		{
			LdArg(0); // 'this'
			if (assignValue)
			{
				assignValue->Accept(*this);
			}

			const auto classType = m_state.analyzer.GetClassType(m_state.currentClass->name);
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

		if (m_state.globalVars.contains(id->name))
		{
			if (isStore)
			{
				StSFld(m_state.globalVars.at(id->name), "IgniGlobalModule", id->name);
			}
			else
			{
				LdSFld(m_state.globalVars.at(id->name), "IgniGlobalModule", id->name);
			}
			return;
		}

		if (const int locIdx = m_state.GetLocalIndex(id->name); locIdx != -1)
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
					re::String baseType = m_state.localTypes[locIdx].Substring(0, m_state.localTypes[locIdx].Length() - 2);
					StElem(GetElemSuffix(baseType));
				}
				else
				{
					LdcI4(0);
					re::String baseType = m_state.localTypes[locIdx].Substring(0, m_state.localTypes[locIdx].Length() - 2);
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

		if (const int argIdx = m_state.GetArgIndex(id->name); argIdx != -1)
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

	static re::String GetBaseClass(const std::vector<std::unique_ptr<ast::AnnotationNode>>& annotations)
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

	void RegisterDelegateFromType(const std::shared_ptr<sem::SemanticType>& type)
	{
		if (const auto funType = std::dynamic_pointer_cast<sem::FunctionType>(type))
		{
			dotnet::LambdaHelper::DelegateInfo info;
			info.className = dotnet::SignatureUtils::GetDelegateName(funType.get());
			info.returnType = funType->returnType ? MapToCIL(funType->returnType->name) : TYPE_VOID;

			for (const auto& pt : funType->paramTypes)
			{
				info.paramTypes.push_back(MapToCIL(pt->name));
			}
			m_lambdaHelper.RegisterDelegate(info);
		}
	}
};

} // namespace igni

#include <IgniLang/Compiler/DotNet/CallProcessor.inl>
#include <IgniLang/Compiler/DotNet/ControlFlowProcessor.inl>
#include <IgniLang/Compiler/DotNet/GlobalProcessor.inl>
#include <IgniLang/Compiler/DotNet/SignatureUtils.inl>