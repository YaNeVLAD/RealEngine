#pragma once

#include <ostream>

namespace igni::codegen
{

template <typename TString>
class Builder
{
public:
	class Scope
	{
	public:
		Scope(Builder* builder, const TString& header)
			: m_builder(builder)
		{
			if (header.size() > 0)
			{
				m_builder->Line(header);
			}
			m_builder->Line("{");
			++m_builder->m_indent;
		}

		~Scope()
		{
			if (m_builder)
			{
				--m_builder->m_indent;
				m_builder->Line("}");
			}
		}

		Scope(const Scope&) = delete;
		Scope& operator=(const Scope&) = delete;

		Scope(Scope&& other) noexcept
			: m_builder(std::exchange(other.m_builder, nullptr))
		{
		}
		Scope& operator=(Scope&& other) noexcept
		{
			if (this == std::addressof(other))
			{
				return *this;
			}

			m_builder = std::exchange(other.m_builder, nullptr);

			return *this;
		}

	private:
		Builder* m_builder;
	};

public:
	explicit Builder(std::ostream& out)
		: m_out(out)
	{
	}

	void Line(const TString& text)
	{
		WriteIndent();
		m_out << text << "\n";
	}

	void EmptyLine()
	{
		m_out << "\n";
	}

	void Include(const TString& path, bool isSystem = false)
	{
		if (isSystem)
		{
			m_out << "#include <" << path << ">\n";
		}
		else
		{
			m_out << "#include \"" << path << "\"\n";
		}
	}

	[[nodiscard]] Scope Block(const TString& header = "")
	{
		return Scope(this, header);
	}

	[[nodiscard]] Scope Namespace(const TString& name)
	{
		return Scope(this, "namespace " + name);
	}

	[[nodiscard]] Scope Function(const TString& returnType, const TString& name, const TString& args)
	{
		return Scope(this, returnType + " " + name + "(" + args + ")");
	}

	[[nodiscard]] Scope If(const TString& condition)
	{
		return Scope(this, "if (" + condition + ")");
	}

	[[nodiscard]] Scope Switch(const TString& variable)
	{
		return Scope(this, "switch (" + variable + ")");
	}

private:
	void WriteIndent()
	{
		m_out << TString(m_indent * 4, ' ');
	}

private:
	std::ostream& m_out;
	int m_indent = 0;
};

} // namespace igni::codegen