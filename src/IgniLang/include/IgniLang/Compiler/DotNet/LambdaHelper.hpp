#pragma once

#include <Core/String.hpp>
#include <IgniLang/AST/AstNodes.hpp>
#include <IgniLang/AST/RecursiveAstVisitor.hpp>
#include <IgniLang/Compiler/DotNet/CILEmitter.hpp>
#include <IgniLang/Compiler/DotNet/TypeMapper.hpp>
#include <IgniLang/Semantic/SemanticAnalyzer.hpp>

#include <unordered_set>
#include <vector>

namespace igni::dotnet
{

struct LambdaData
{
	const ast::LambdaExpr* node;
	re::String className;
	std::vector<re::String> captureTypes;
	std::vector<bool> isByRef;
};

class LambdaHelper
{
public:
	LambdaHelper(CILEmitter& emitter, const sem::SemanticAnalyzer& semantics)
		: m_emitter(emitter)
		, m_semanticAnalyzer(semantics)
	{
	}

	std::unordered_set<re::String> ScanCaptures(const ast::Block* body)
	{
		m_capturedLocals.clear();
		if (body)
		{
			CaptureScanner scanner(m_capturedLocals);
			body->Accept(scanner);
		}
		return m_capturedLocals;
	}

	void RegisterLambda(const LambdaData& data)
	{
		m_lambdas.push_back(data);
	}

	const LambdaData* GetCurrentLambda() const
	{
		return m_currentLambdaData;
	}

	bool IsCaptured(const re::String& name) const
	{
		return m_capturedLocals.contains(name);
	}

	void GenerateAllClasses(std::ostream& out, const std::function<void(const re::String&, const ast::Block*, bool, const std::vector<std::unique_ptr<ast::ParameterNode>>&)>& methodBuilder)
	{
		for (std::size_t i = 0; i < m_lambdas.size(); ++i)
		{
			LambdaData data = m_lambdas[i];

			m_emitter.SetStream(out);
			m_emitter.BeginClass(data.className, "[mscorlib]System.Object");

			for (std::size_t j = 0; j < data.captureTypes.size(); ++j)
			{
				re::String fieldType = data.captureTypes[j];
				if (data.isByRef[j])
				{
					fieldType += "[]";
				}
				m_emitter.Field(fieldType, data.node->captures[j], false);
			}

			re::String ctorSig = ".method public hidebysig specialname rtspecialname instance void .ctor(";
			for (std::size_t j = 0; j < data.captureTypes.size(); ++j)
			{
				ctorSig += data.captureTypes[j] + (data.isByRef[j] ? "[]" : "") + (j < data.captureTypes.size() - 1 ? ", " : "");
			}
			ctorSig += ") cil managed";

			m_emitter.BeginMethodBody(ctorSig, false);
			m_emitter.LdArg0();
			m_emitter.Call("instance void [mscorlib]System.Object::.ctor()");

			for (std::size_t j = 0; j < data.captureTypes.size(); ++j)
			{
				m_emitter.LdArg0();
				m_emitter.LdArg(static_cast<int>(j + 1));
				m_emitter.StFld(data.captureTypes[j] + (data.isByRef[j] ? "[]" : ""), data.className, data.node->captures[j]);
			}
			m_emitter.Ret();
			m_emitter.EndMethodBody();

			m_currentLambdaData = &data;
			const re::String retType = data.node->returnType ? TypeMapper::MapAstType(data.node->returnType.get()) : "void";
			const re::String sig = ".method public hidebysig instance " + retType + " Invoke(" + TypeMapper::BuildParamSignature(data.node->parameters, false) + ") cil managed";

			methodBuilder(sig, data.node->body.get(), false, data.node->parameters);

			m_currentLambdaData = nullptr;
			m_emitter.SetStream(out);
			m_emitter.EndClass();
		}
	}

private:
	CILEmitter& m_emitter;
	const sem::SemanticAnalyzer& m_semanticAnalyzer;
	std::vector<LambdaData> m_lambdas;
	const LambdaData* m_currentLambdaData = nullptr;
	std::unordered_set<re::String> m_capturedLocals;

	class CaptureScanner : public ast::RecursiveAstVisitor
	{
	public:
		explicit CaptureScanner(std::unordered_set<re::String>& captures)
			: m_captures(captures)
		{
		}

		void Visit(const ast::LambdaExpr* node) override
		{
			for (const auto& cap : node->captures)
			{
				m_captures.insert(cap);
			}

			RecursiveAstVisitor::Visit(node);
		}

	private:
		std::unordered_set<re::String>& m_captures;
	};
};

} // namespace igni::dotnet