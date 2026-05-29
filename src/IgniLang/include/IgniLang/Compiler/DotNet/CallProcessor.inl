#pragma once

namespace igni::dotnet
{

inline void CallProcessor::ProcessCallExpr(const ast::CallExpr* node) const
{
	if (TryProcessIndirectDelegate(node))
	{
		return;
	}

	const auto& callInfo = m_gen.m_state.analyzer.GetBindings().callInfo.at(node);
	if (TryProcessInlineOpcode(node, callInfo))
	{
		return;
	}

	if (callInfo.dispatchMode == CallDispatchType::Indirect)
	{
		ProcessIndirectCall(node, callInfo);
		return;
	}

	if (callInfo.dispatchMode == CallDispatchType::Native)
	{
		ProcessNativeCall(node, callInfo);
		return;
	}

	if (callInfo.isConstructorCall)
	{
		ProcessConstructorCall(node, callInfo);
		return;
	}

	if (callInfo.dispatchMode == CallDispatchType::Virtual)
	{
		ProcessVirtualCall(node, callInfo);
		return;
	}

	ProcessGlobalOrHoistedCall(node, callInfo);
}

inline bool CallProcessor::TryProcessIndirectDelegate(const ast::CallExpr* node) const
{
	if (m_gen.m_state.analyzer.GetBindings().callInfo.contains(node))
	{
		return false;
	}

	const auto calleeType = m_gen.m_state.analyzer.GetBindings().GetExpressionType(node->callee.get());
	if (const auto funType = std::dynamic_pointer_cast<sem::FunctionType>(calleeType))
	{
		node->callee->Accept(m_gen);
		for (const auto& arg : node->arguments)
		{
			if (arg)
			{
				arg->Accept(m_gen);
			}
		}

		const re::String delegateName = SignatureUtils::GetDelegateName(funType.get());
		const re::String invokeSig = SignatureUtils::BuildCilSignature(funType.get(), delegateName, "Invoke", true, false);
		m_gen.CallVirtual(invokeSig);
		return true;
	}
	return false;
}

inline bool CallProcessor::TryProcessInlineOpcode(const ast::CallExpr* node, const CallInfo& callInfo) const
{
	const auto inlineOp = ast::AnnotationUtils::GetAnnotationArg(callInfo.target->annotations, "DotNetOpcode");
	if (!inlineOp)
	{
		return false;
	}

	const re::String op = *inlineOp;
	if (op == "ldlen")
	{
		if (const auto memAccess = dynamic_cast<const ast::MemberAccessExpr*>(node->callee.get()))
		{
			memAccess->object->Accept(m_gen);
		}
		m_gen.LdLen();
		m_gen.ConvI8();
	}
	else if (op == "ldelem")
	{
		ProcessLdelem(node, callInfo);
	}
	else if (op == "stelem")
	{
		ProcessStelem(node, callInfo);
	}
	else if (op == "newarr")
	{
		ProcessArguments(node, callInfo, true);
		re::String elemType = "class [mscorlib]System.Object";
		if (const auto classType = std::dynamic_pointer_cast<sem::ClassType>(callInfo.target->returnType))
		{
			if (!classType->typeArguments.empty())
			{
				elemType = m_gen.MapToCIL(classType->typeArguments[0]->name);
			}
		}

		re::String newArrInstr = "newarr ";
		newArrInstr += elemType;
		m_gen.Emit(newArrInstr);
	}
	else
	{
		ProcessArguments(node, callInfo, false);
		m_gen.Emit(op);
	}
	return true;
}

inline void CallProcessor::ProcessLdelem(const ast::CallExpr* node, const CallInfo& callInfo) const
{
	if (const auto memAccess = dynamic_cast<const ast::MemberAccessExpr*>(node->callee.get()))
	{
		memAccess->object->Accept(m_gen);
	}
	if (!node->arguments.empty() && node->arguments[0])
	{
		node->arguments[0]->Accept(m_gen);
	}
	m_gen.ConvI4();

	re::String elemType = "System.Object";
	if (const auto classType = std::dynamic_pointer_cast<sem::ClassType>(callInfo.target->paramTypes[0]))
	{
		if (!classType->typeArguments.empty())
		{
			elemType = classType->typeArguments[0]->name;
		}
	}

	re::String ldelemInstr = "ldelem.";
	ldelemInstr += m_gen.GetElemSuffix(elemType);
	m_gen.Emit(ldelemInstr);
}

inline void CallProcessor::ProcessStelem(const ast::CallExpr* node, const CallInfo& callInfo) const
{
	if (const auto memAccess = dynamic_cast<const ast::MemberAccessExpr*>(node->callee.get()))
	{
		memAccess->object->Accept(m_gen);
	}
	if (!node->arguments.empty() && node->arguments[0])
	{
		node->arguments[0]->Accept(m_gen);
	}
	m_gen.ConvI4();
	if (node->arguments.size() > 1 && node->arguments[1])
	{
		node->arguments[1]->Accept(m_gen);
	}

	re::String elemType = "System.Object";
	if (const auto classType = std::dynamic_pointer_cast<sem::ClassType>(callInfo.target->paramTypes[0]))
	{
		if (!classType->typeArguments.empty())
		{
			elemType = classType->typeArguments[0]->name;
		}
	}

	re::String stelemInstr = "stelem.";
	stelemInstr += m_gen.GetElemSuffix(elemType);
	m_gen.Emit(stelemInstr);
}

inline void CallProcessor::ProcessArguments(const ast::CallExpr* node, const CallInfo& callInfo, const bool ignoreVararg) const
{
	if (callInfo.target->isVararg && !ignoreVararg)
	{
		ProcessVarargArguments(node, callInfo);
	}
	else
	{
		for (const auto& arg : node->arguments)
		{
			if (arg)
			{
				arg->Accept(m_gen);
			}
		}
	}
}

inline void CallProcessor::ProcessVarargArguments(const ast::CallExpr* node, const CallInfo& callInfo) const
{
	const std::size_t normalCount = callInfo.target->paramTypes.size() - 1;
	for (std::size_t i = 0; i < normalCount; ++i)
	{
		if (node->arguments[i])
		{
			node->arguments[i]->Accept(m_gen);
		}
	}

	const std::size_t varargCount = node->arguments.size() - normalCount;
	m_gen.LdcI4(static_cast<int>(varargCount));

	const re::String elemType = callInfo.target->paramTypes.back()->name;
	re::String cilElemType = m_gen.MapToCIL(elemType);
	if (cilElemType.Find("[]") != re::String::NPos)
	{
		cilElemType = cilElemType.Substring(0, cilElemType.Length() - 2);
	}

	m_gen.NewArr(cilElemType);
	PackVarargsLoop(node, normalCount, varargCount, elemType);
}

inline void CallProcessor::PackVarargsLoop(const ast::CallExpr* node, const std::size_t normalCount, const std::size_t varargCount, const re::String& elemType) const
{
	for (std::size_t i = 0; i < varargCount; ++i)
	{
		m_gen.Dup();
		m_gen.LdcI4(static_cast<int>(i));

		if (node->arguments[normalCount + i])
		{
			node->arguments[normalCount + i]->Accept(m_gen);
			if (elemType == "Any" || elemType == "System.Object")
			{
				if (const auto argSemType = m_gen.m_state.analyzer.GetBindings().GetExpressionType(node->arguments[normalCount + i].get()))
				{
					if (argSemType->name == "System.Int64")
					{
						m_gen.Box("[mscorlib]System.Int64");
					}
					else if (argSemType->name == "System.Double")
					{
						m_gen.Box("[mscorlib]System.Double");
					}
					else if (argSemType->name == "System.Boolean")
					{
						m_gen.Box("[mscorlib]System.Boolean");
					}
				}
			}
		}
		m_gen.StElem(m_gen.GetElemSuffix(elemType));
	}
}

inline void CallProcessor::ProcessIndirectCall(const ast::CallExpr* node, const CallInfo& callInfo) const
{
	if (node->callee)
	{
		node->callee->Accept(m_gen);
	}
	ProcessArguments(node, callInfo, false);

	const re::String retType = callInfo.target && callInfo.target->returnType ? m_gen.MapToCIL(callInfo.target->returnType->name) : re::String("void");
	re::String invokeSig = retType;
	invokeSig += " ";
	invokeSig += callInfo.target->name;
	invokeSig += "::Invoke(";
	invokeSig += SignatureUtils::BuildTypeSignature(callInfo.target->paramTypes, 0, callInfo.target->isVararg);
	invokeSig += ")";

	m_gen.CallVirtual(invokeSig);
}

inline void CallProcessor::ProcessNativeCall(const ast::CallExpr* node, const CallInfo& callInfo) const
{
	if (const auto dotnetMethod = ast::AnnotationUtils::GetAnnotationArg(callInfo.target->annotations, "DotNetMethod"))
	{
		ProcessArguments(node, callInfo, true);
		m_gen.Call(*dotnetMethod);
		return;
	}
	ProcessArguments(node, callInfo, false);
	m_gen.Call(callInfo.target->name);
}

inline void CallProcessor::ProcessConstructorCall(const ast::CallExpr* node, const CallInfo& callInfo) const
{
	ProcessArguments(node, callInfo, false);

	re::String ctorSig;
	ctorSig += callInfo.mangledClassName;
	ctorSig += "::.ctor(";
	ctorSig += SignatureUtils::BuildTypeSignature(callInfo.target->paramTypes, 1, callInfo.target->isVararg);
	ctorSig += ")";

	m_gen.NewObj(ctorSig);
}

inline void CallProcessor::ProcessVirtualCall(const ast::CallExpr* node, const CallInfo& callInfo) const
{
	if (const auto memAccess = dynamic_cast<const ast::MemberAccessExpr*>(node->callee.get()))
	{
		memAccess->object->Accept(m_gen);
	}
	else if (callInfo.isImplicitThisCall)
	{
		m_gen.LdArg0();
	}

	ProcessArguments(node, callInfo, false);
	const re::String virtSig = SignatureUtils::BuildCilSignature(callInfo.target.get(), callInfo.target->paramTypes[0]->name, callInfo.asmLabel, false, true);
	m_gen.CallVirtual(virtSig);
}

inline void CallProcessor::ProcessGlobalOrHoistedCall(const ast::CallExpr* node, const CallInfo& callInfo) const
{
	re::String cleanName = callInfo.asmLabel;
	if (const std::size_t firstAt = cleanName.Find('@'); firstAt != re::String::NPos)
	{
		cleanName = cleanName.Substring(0, firstAt);
	}

	if (cleanName.Length() > 0)
	{
		if (const std::size_t underPos = cleanName.Find('_'); underPos != re::String::NPos)
		{
			const re::String prefix = cleanName.Substring(0, underPos);
			const re::String suffix = cleanName.Substring(underPos + 1, cleanName.Length() - underPos - 1);

			if (prefix == suffix && m_gen.m_state.currentClass && callInfo.dispatchMode != CallDispatchType::Virtual)
			{
				m_gen.LdArg0();
				ProcessArguments(node, callInfo, false);

				re::String callSig = "instance void ";
				callSig += prefix;
				callSig += "::.ctor(";
				callSig += SignatureUtils::BuildTypeSignature(callInfo.target->paramTypes, 1, callInfo.target->isVararg);
				callSig += ")";

				m_gen.Call(callSig);
				return;
			}
		}
	}

	if (const auto memAccess = dynamic_cast<const ast::MemberAccessExpr*>(node->callee.get()))
	{
		memAccess->object->Accept(m_gen);
	}
	else if (callInfo.isImplicitThisCall)
	{
		m_gen.LdArg0();
	}

	ProcessArguments(node, callInfo, false);
	const re::String globalSig = SignatureUtils::BuildCilSignature(callInfo.target.get(), "IgniGlobalModule", callInfo.asmLabel, false, false);
	m_gen.Call(globalSig);
}

} // namespace igni::dotnet