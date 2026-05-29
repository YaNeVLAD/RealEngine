#pragma once

namespace igni::dotnet
{

inline void ControlFlowProcessor::ProcessForStmt(const ast::ForStmt* node) const
{
	if (node->isForEach)
	{
		ProcessForEachLoop(node);
	}
	else
	{
		ProcessStandardForLoop(node);
	}
}

inline void ControlFlowProcessor::ProcessForEachLoop(const ast::ForStmt* node) const
{
	const std::size_t currentLabel = m_gen.m_state.labelCount++;
	const std::string startLabel = "L_foreach_start_" + std::to_string(currentLabel);
	const std::string endLabel = "L_foreach_end_" + std::to_string(currentLabel);

	const std::string arrName = "_arr_" + std::to_string(currentLabel);
	m_gen.DeclareLocal(arrName, node->startExpr.get());
	const int arrIdx = m_gen.m_state.GetLocalIndex(arrName);

	const std::string idxName = "_idx_" + std::to_string(currentLabel);
	m_gen.m_state.locals.emplace_back(idxName);
	m_gen.m_state.localTypes.emplace_back("int64");
	const int idxIdx = static_cast<int>(m_gen.m_state.locals.size() - 1);

	m_gen.LdcI8(0);
	m_gen.StLoc(idxIdx);

	re::String pureSemType = "System.Object";
	re::String elemCilType = "class [mscorlib]System.Object";

	if (const auto collSemType = m_gen.m_state.analyzer.GetBindings().GetExpressionType(node->startExpr.get()))
	{
		if (const auto classType = std::dynamic_pointer_cast<sem::ClassType>(collSemType))
		{
			if (!classType->typeArguments.empty())
			{
				pureSemType = classType->typeArguments[0]->name;
				elemCilType = m_gen.MapToCIL(pureSemType);
			}
		}
	}

	m_gen.m_state.locals.push_back(node->iteratorName);
	m_gen.m_state.localTypes.push_back(elemCilType);
	const int iterIdx = static_cast<int>(m_gen.m_state.locals.size() - 1);

	m_gen.MarkLabel(startLabel);
	m_gen.LdLoc(idxIdx);
	m_gen.LdLoc(arrIdx);
	m_gen.LdLen();
	m_gen.ConvI8();
	m_gen.Bge(endLabel);

	m_gen.LdLoc(arrIdx);
	m_gen.LdLoc(idxIdx);
	m_gen.ConvI4();
	m_gen.LdElem(m_gen.GetElemSuffix(pureSemType));
	m_gen.StLoc(iterIdx);

	if (node->body)
	{
		node->body->Accept(m_gen);
	}

	m_gen.LdLoc(idxIdx);
	m_gen.LdcI8(1);
	m_gen.Emit("add");
	m_gen.StLoc(idxIdx);

	m_gen.Br(startLabel);
	m_gen.MarkLabel(endLabel);
}

inline void ControlFlowProcessor::ProcessStandardForLoop(const ast::ForStmt* node) const
{
	const std::string startLabel = "L_for_start_" + std::to_string(m_gen.m_state.labelCount);
	const std::string endLabel = "L_for_end_" + std::to_string(m_gen.m_state.labelCount++);

	m_gen.DeclareLocal(node->iteratorName, node->startExpr.get());
	const int iterIdx = m_gen.m_state.GetLocalIndex(node->iteratorName);

	const std::string limitName = "_for_limit_" + std::to_string(m_gen.m_state.labelCount);
	m_gen.DeclareLocal(limitName, node->endExpr.get());
	const int limitIdx = m_gen.m_state.GetLocalIndex(limitName);

	m_gen.MarkLabel(startLabel);
	m_gen.LdLoc(iterIdx);
	m_gen.LdLoc(limitIdx);
	m_gen.Bgt(endLabel);

	if (node->body)
	{
		node->body->Accept(m_gen);
	}

	m_gen.LdLoc(iterIdx);
	m_gen.LdcI8(1);
	m_gen.Emit("add");
	m_gen.StLoc(iterIdx);

	m_gen.Br(startLabel);
	m_gen.MarkLabel(endLabel);
}

inline void ControlFlowProcessor::ProcessIfStmt(const ast::IfStmt* node) const
{
	const std::string elseLabel = "L_else_" + std::to_string(m_gen.m_state.labelCount);
	const std::string endLabel = "L_end_" + std::to_string(m_gen.m_state.labelCount++);

	if (node->condition)
	{
		node->condition->Accept(m_gen);
	}
	m_gen.BrFalse(node->elseBranch ? elseLabel : endLabel);

	if (node->thenBranch)
	{
		node->thenBranch->Accept(m_gen);
	}

	if (node->elseBranch)
	{
		m_gen.Br(endLabel);
		m_gen.MarkLabel(elseLabel);
		node->elseBranch->Accept(m_gen);
	}
	m_gen.MarkLabel(endLabel);
}

inline void ControlFlowProcessor::ProcessWhileStmt(const ast::WhileStmt* node) const
{
	const std::string startLabel = "L_while_start_" + std::to_string(m_gen.m_state.labelCount);
	const std::string endLabel = "L_while_end_" + std::to_string(m_gen.m_state.labelCount++);

	m_gen.MarkLabel(startLabel);
	if (node->condition)
	{
		node->condition->Accept(m_gen);
	}
	m_gen.BrFalse(endLabel);

	if (node->body)
	{
		node->body->Accept(m_gen);
	}

	m_gen.Br(startLabel);
	m_gen.MarkLabel(endLabel);
}

} // namespace igni::dotnet