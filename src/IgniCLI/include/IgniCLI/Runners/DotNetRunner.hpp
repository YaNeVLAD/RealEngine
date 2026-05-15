#pragma once

#include <IgniCLI/Runners/IRunner.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>

namespace igni::cli
{

class DotNetRunner : public IRunner
{
	static constexpr auto WIN_PATH = R"(C:\Windows\Microsoft.NET\Framework64\v4.0.30319\ilasm.exe)";
	static constexpr auto IL_FILE = "temp_output.il";
	static constexpr auto EXE_FILE = "temp_output.exe";

public:
	int Run(const std::string& generatedCode) override
	{
		{
			std::ofstream out(IL_FILE);
			out << generatedCode;
		}

		std::cout << "[Info] Assembling .NET binary..." << std::endl;

		std::string ilasmPath = "ilasm";
		if (std::filesystem::exists(WIN_PATH))
		{
			ilasmPath = WIN_PATH;
		}

		const std::string compileCmd = "\"" + ilasmPath + "\" " + IL_FILE + " /exe /output=" + EXE_FILE + " /quiet";
		if (const int res = std::system(compileCmd.c_str()); res != 0)
		{
			std::cerr << "[Error] ilasm failed to assemble the program.\n";
			return res;
		}

		std::cout << "[Info] Running .NET Program:\n------------------\n";

		// На современных системах .exe может требовать 'dotnet' для запуска,
		// если это не self-contained. Но ilasm обычно делает обычный PE.
		const int runRes = std::system(EXE_FILE);
		std::cout << "\n------------------\n[Info] Program finished with code " << runRes << std::endl;

		std::filesystem::remove(IL_FILE);
		std::filesystem::remove(EXE_FILE);

		return runRes;
	}
};

} // namespace igni::cli