#pragma once

#include <Core/String.hpp>

#include <iostream>
#include <string>
#include <vector>

namespace igni
{

enum class DiagnosticLevel
{
	Error,
	Warning,
	Info
};

struct Diagnostic
{
	DiagnosticLevel level;
	re::String message;

	std::size_t line;
	std::size_t column;
	std::size_t offset;
	std::size_t length;
};

class DiagnosticEngine
{
public:
	void ReportError(re::String message, const std::size_t line, const std::size_t col, const std::size_t offset, const std::size_t len = 1)
	{
		m_diagnostics.push_back({ DiagnosticLevel::Error, std::move(message), line, col, offset, len });
	}

	[[nodiscard]] const std::vector<Diagnostic>& GetDiagnostics() const
	{
		return m_diagnostics;
	}

	[[nodiscard]] bool HasErrors() const
	{
		return !m_diagnostics.empty();
	}

	void PrintToConsole() const
	{
		for (const auto& diag : m_diagnostics)
		{
			std::cerr << "[Line: " << diag.line << ", Col: " << diag.column << "] Error: " << diag.message << "\n";
		}
	}

private:
	std::vector<Diagnostic> m_diagnostics;
};

} // namespace igni