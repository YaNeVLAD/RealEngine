#pragma once

#include <Core/FileSystem.hpp>

#include <cstdlib>
#include <filesystem>
#include <iostream>

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

		// TODO: Move to configuration file
		const std::string compilerPath = ".\\igni-cli.exe";
		for (const auto& entry : fs::recursive_directory_iterator(scriptsDir))
		{
			if (entry.is_regular_file() && entry.path().extension() == ".igni")
			{
				const fs::path& sourcePath = entry.path();

				fs::path relativePath = fs::relative(sourcePath, scriptsDir);
				fs::path binaryPath = binDir / relativePath;
				binaryPath.replace_extension(".rbc");

				bool needsCompilation = true;
				if (fs::exists(binaryPath))
				{
					auto srcTime = fs::last_write_time(sourcePath);
					auto binTime = fs::last_write_time(binaryPath);
					if (binTime >= srcTime)
					{
						needsCompilation = false;
					}
				}

				if (needsCompilation)
				{
					std::string command = compilerPath + " --rvm --dll \"" + sourcePath.string() + "\" -o \"" + binaryPath.string() + "\"";

					std::cout << "[ScriptCompiler] Compiling " << sourcePath.filename().string() << "...\n";

					if (const int exitCode = std::system(command.c_str()); exitCode == 0)
					{
						std::cout << "[ScriptCompiler] Success: " << binaryPath.filename().string() << "\n";
					}
					else
					{
						std::cerr << "[ScriptCompiler] Failed to compile: " << sourcePath.filename().string() << " (Exit code: " << exitCode << ")\n";
					}
				}
			}
		}
	}
};

} // namespace re::scripting