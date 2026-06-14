#pragma once

#include <Core/FileSystem.hpp>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace re::scripting
{

class ScriptCompiler
{
public:
	static void CompileAllModified()
	{
		using namespace re;
		namespace fs = std::filesystem;

		const fs::path scriptsDir = file_system::ScriptsPath("src").Str().Data();
		const fs::path binDir = file_system::ScriptsPath("bin").Str().Data();

		if (!fs::exists(scriptsDir))
		{
			fs::create_directories(scriptsDir);
			return;
		}

		if (!fs::exists(binDir))
		{
			fs::create_directories(binDir);
		}

		const fs::path binaryPath = binDir / "game_scripts.rbc";
		const std::string compilerPath = "igni-cli";

		std::vector<std::string> sourceFiles;
		bool needsCompilation = !fs::exists(binaryPath);
		const auto binTime = needsCompilation ? fs::file_time_type::min() : fs::last_write_time(binaryPath);

		for (const auto& entry : fs::recursive_directory_iterator(scriptsDir))
		{
			if (entry.is_regular_file() && entry.path().extension() == ".igni")
			{
				sourceFiles.push_back(entry.path().string());

				if (!needsCompilation && fs::last_write_time(entry.path()) > binTime)
				{
					needsCompilation = true;
				}
			}
		}

		if (needsCompilation && !sourceFiles.empty())
		{
			std::cout << "[ScriptCompiler] Changes detected. Building game scripts...\n";

			std::string command = compilerPath + " --rvm --dll ";

			for (const auto& file : sourceFiles)
			{
				command += "\"" + file + "\" ";
			}

			command += "-o \"" + binaryPath.string() + "\"";

			std::cout << "[ScriptCompiler] Executing: " << command << std::endl;

			if (const auto exitCode = std::system(command.c_str()); exitCode == 0)
			{
				std::cout << "[ScriptCompiler] Success: game_scripts.rbc generated" << std::endl;
			}
			else
			{
				std::cerr << "[ScriptCompiler] Build failed (Exit code: " << exitCode << ")" << std::endl;
			}
		}
		else if (sourceFiles.empty())
		{
			std::cout << "[ScriptCompiler] No scripts found in " << scriptsDir << std::endl;
		}
		else
		{
			std::cout << "[ScriptCompiler] Scripts are up to date." << std::endl;
		}
	}
};

} // namespace re::scripting