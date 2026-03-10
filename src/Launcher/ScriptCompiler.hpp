#pragma once

#include <Core/FileSystem.hpp>
#include <RVM/Assembler.hpp>
#include <RVM/Chunk.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace launcher
{

inline void CompileScripts()
{
	using namespace re;
	namespace fs = std::filesystem;

	fs::path scriptsDir = file_system::AssetsPath("scripts").Str().Data();

	if (!fs::exists(scriptsDir))
	{
		fs::create_directories(scriptsDir);
		return;
	}

	rvm::Assembler assembler;
	for (const auto& entry : fs::recursive_directory_iterator(scriptsDir))
	{
		if (entry.is_regular_file() && entry.path().extension() == ".rbs")
		{
			fs::path sourcePath = entry.path();
			fs::path binaryPath = sourcePath;
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
				if (std::ifstream file(sourcePath); file.is_open())
				{
					std::stringstream ss;
					ss << file.rdbuf();
					std::string sourceCode = ss.str();

					if (rvm::Chunk chunk; assembler.Compile(sourceCode, chunk))
					{
						if (chunk.SaveToFile(binaryPath.string()))
						{
							std::cout << "[RVM] Compiled: " << sourcePath.filename().string() << "\n";
						}
					}
					else
					{
						std::cerr << "[RVM] Compile error in: " << sourcePath.filename().string() << "\n";
					}
				}
			}
		}
	}
}

} // namespace launcher