#pragma once

#include <Core/Config.hpp>
#include <Core/String.hpp>

#include <array>

namespace re
{

struct ProcessResult
{
	int exitCode;
	String stdOut;
};

namespace Process
{

inline ProcessResult Run(const String& command)
{
	const std::string cmd = command.ToString();
	std::string result;
	int exitCode = -1;

#ifdef RE_SYSTEM_WINDOWS
	FILE* pipe = _popen(cmd.c_str(), "r");
#else
	FILE* pipe = popen(cmd.c_str(), "r");
#endif

	if (!pipe)
	{
		throw std::runtime_error("Failed to start process: " + cmd);
	}

	std::array<char, 256> buffer;
	while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
	{
		result += buffer.data();
	}

#ifdef RE_SYSTEM_WINDOWS
	exitCode = _pclose(pipe);
#else
	exitCode = pclose(pipe);
#endif

	return { exitCode, result };
}

} // namespace Process

} // namespace re