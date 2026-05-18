#pragma once

#include <Core/FlatMap.hpp>
#include <IgniCLI/BuildType.hpp>
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
	static constexpr auto IL_FILE = "IgniProgram.il";
	static constexpr auto EXE_FILE_NAME = "IgniProgram";

public:
	explicit DotNetRunner(const BuildType buildType)
		: m_buildType(buildType)
	{
	}

	int Run(const std::string& generatedCode) override
	{
		using namespace std::literals;

		static constexpr auto BUILD_TYPE_EXT = re::MakeFlatMap<enum BuildType, std::string_view>({
			{ BuildType::DynamicLibrary, ".dll"sv },
			{ BuildType::Executable, ".exe"sv },
		});
		const auto ext = BUILD_TYPE_EXT[m_buildType];
		const auto target = ext->substr(1);

		{
			std::ofstream out(IL_FILE, std::ios::binary);

			// UTF-8 BOM
			constexpr unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
			out.write(reinterpret_cast<const char*>(bom), sizeof(bom));

			out << generatedCode;
		}

		std::cout << "[Info] Assembling .NET binary..." << std::endl;

		std::string ilasmPath = "ilasm";
		if (std::filesystem::exists(WIN_PATH))
		{
			ilasmPath = WIN_PATH;
		}

		const auto outFile = std::string(EXE_FILE_NAME) + ext->data();
		const auto compileCmd = std::format("{} {} /{} /output={} /quiet", ilasmPath, IL_FILE, target, outFile);
		if (const int res = std::system(compileCmd.c_str()); res != 0)
		{
			std::cerr << "[Error] ilasm failed to assemble the program.\n";
			return res;
		}
		std::cout << "[Success] Generated " << outFile << " successfully!\n";

		int res = 0;
		const bool shouldRun = m_buildType == BuildType::Executable;
		if (shouldRun)
		{
			std::cout << "------------------\n[Info] Running " << outFile << ":\n------------------\n";
			res = std::system(outFile.c_str());
			std::cout << "------------------\n[Info] Program finished with code " << res << "\n";

			std::filesystem::remove(outFile);
		}
		std::filesystem::remove(IL_FILE);

		return res;
	}

private:
	BuildType m_buildType;
};

} // namespace igni::cli