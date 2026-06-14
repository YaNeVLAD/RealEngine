#pragma once

#include <Core/HashedString.hpp>
#include <Core/String.hpp>
#include <IgniLang/AST/AstNodes.hpp>

#include <iostream>

namespace igni
{

class AsmCodeGenerator final : public ast::BaseAstVisitor
{
public:
	explicit AsmCodeGenerator(std::ostream& out)
		: m_out(out)
	{
	}

	void Generate(const ast::Program* program)
	{
		m_out << "global main\n";
		m_out << "section .text\n";

		for (const auto& stmt : program->statements)
		{
			if (stmt)
			{
				stmt->Accept(*this);
			}
		}
	}

	void Visit(const ast::FunDecl* node) override
	{
		m_out << node->name << ":\n";

		m_out << "    push rbp\n";
		m_out << "    mov rbp, rsp\n";

		if (node->body)
		{
			node->body->Accept(*this);
		}

		m_out << "    mov rsp, rbp\n";
		m_out << "    pop rbp\n";
		m_out << "    ret\n\n";
	}

	void Visit(const ast::Block* node) override
	{
		for (const auto& stmt : node->statements)
		{
			if (stmt)
			{
				stmt->Accept(*this);
			}
		}
	}

	void Visit(const ast::LiteralExpr* node) override
	{
		if (node->token.type == TokenType::IntConst)
		{
			m_out << "    push " << node->token.lexeme << "\n";
		}
		else if (node->token.type == TokenType::KwTrue)
		{
			m_out << "    push 1\n";
		}
		else if (node->token.type == TokenType::KwFalse)
		{
			m_out << "    push 0\n";
		}
	}

	void Visit(const ast::UnaryExpr* node) override
	{
		using namespace re::literals;

		if (node->operand)
		{
			node->operand->Accept(*this);
		}

		m_out << "    pop rax\n";

		switch (node->op.Hashed())
		{
		case "-"_hs:
			m_out << "    neg rax\n";
			break;
		case "!"_hs:
			m_out << "    cmp rax, 0\n";
			m_out << "    sete al\n";
			m_out << "    movzx rax, al\n";
			break;
		case "~"_hs:
			m_out << "    not rax\n";
			break;
		default:
			throw std::runtime_error("unknown unary operator");
		}

		m_out << "    push rax\n";
	}

	void Visit(const ast::BinaryExpr* node) override
	{
		using namespace re::literals;

		const auto op = node->op.Hashed();

		if (op == "and"_hs)
		{
			node->left->Accept(*this);
			m_out << "    pop rax\n";
			m_out << "    cmp rax, 0\n";
			m_out << "    je .L_false_" << m_labelCount << "\n";
			node->right->Accept(*this);
			m_out << "    pop rax\n";
			m_out << "    cmp rax, 0\n";
			m_out << "    je .L_false_" << m_labelCount << "\n";
			m_out << "    push 1\n";
			m_out << "    jmp .L_end_" << m_labelCount << "\n";
			m_out << ".L_false_" << m_labelCount << ":\n";
			m_out << "    push 0\n";
			m_out << ".L_end_" << m_labelCount << ":\n";
			m_labelCount++;
			return;
		}

		if (op == "or"_hs)
		{
			node->left->Accept(*this);
			m_out << "    pop rax\n";
			m_out << "    cmp rax, 0\n";
			m_out << "    jne .L_true_" << m_labelCount << "\n";
			node->right->Accept(*this);
			m_out << "    pop rax\n";
			m_out << "    cmp rax, 0\n";
			m_out << "    jne .L_true_" << m_labelCount << "\n";
			m_out << "    push 0\n";
			m_out << "    jmp .L_end_" << m_labelCount << "\n";
			m_out << ".L_true_" << m_labelCount << ":\n";
			m_out << "    push 1\n";
			m_out << ".L_end_" << m_labelCount << ":\n";
			m_labelCount++;
			return;
		}

		if (node->left)
		{
			node->left->Accept(*this);
		}
		if (node->right)
		{
			node->right->Accept(*this);
		}

		m_out << "    pop rbx\n"; // RHS
		m_out << "    pop rax\n"; // LHS

		switch (op)
		{
		case "+"_hs:
			m_out << "    add rax, rbx\n";
			break;
		case "-"_hs:
			m_out << "    sub rax, rbx\n";
			break;
		case "*"_hs:
			m_out << "    imul rax, rbx\n";
			break;
		case "/"_hs:
		case "%"_hs:
			m_out << "    cqo\n";
			m_out << "    idiv rbx\n";
			if (op == "%"_hs)
			{
				m_out << "    mov rax, rdx\n";
			}
			break;

		case "=="_hs:
			m_out << "    cmp rax, rbx\n";
			m_out << "    sete al\n";
			m_out << "    movzx rax, al\n";
			break;
		case "!="_hs:
			m_out << "    cmp rax, rbx\n";
			m_out << "    setne al\n";
			m_out << "    movzx rax, al\n";
			break;
		case "<"_hs:
			m_out << "    cmp rax, rbx\n";
			m_out << "    setl al\n";
			m_out << "    movzx rax, al\n";
			break;
		case ">"_hs:
			m_out << "    cmp rax, rbx\n";
			m_out << "    setg al\n";
			m_out << "    movzx rax, al\n";
			break;
		case "<="_hs:
			m_out << "    cmp rax, rbx\n";
			m_out << "    setle al\n";
			m_out << "    movzx rax, al\n";
			break;
		case ">="_hs:
			m_out << "    cmp rax, rbx\n";
			m_out << "    setge al\n";
			m_out << "    movzx rax, al\n";
			break;
		default:
			throw std::runtime_error("unknown binary operator");
		}

		m_out << "    push rax\n";
	}

	void Visit(const ast::ExprStmt* node) override
	{
		if (node->expr)
		{
			node->expr->Accept(*this);
		}

		m_out << "    add rsp, 8\n";
	}

private:
	std::ostream& m_out;
	std::size_t m_labelCount = 0;
};

} // namespace igni