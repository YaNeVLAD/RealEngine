#pragma once

#include <Core/String.hpp>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace igni::backend
{

struct BackendInfo
{
	re::String name;
	re::String flag;
	re::String executablePath;
};

class BackendRegistry
{
public:
	static std::vector<BackendInfo> Load(const re::String& cliExecutablePath)
	{
		std::vector<BackendInfo> backends;

		std::filesystem::path cliPath(cliExecutablePath);
		std::filesystem::path registryPath = cliPath.parent_path() / "backends.json";

		if (!std::filesystem::exists(registryPath))
		{
			return backends;
		}

		std::ifstream file(registryPath);
		if (!file.is_open())
		{
			throw std::runtime_error("Failed to open " + registryPath.string());
		}

		try
		{
			nlohmann::json json;
			file >> json;

			if (json.contains("backends") && json["backends"].is_array())
			{
				for (const auto& item : json["backends"])
				{
					BackendInfo info;
					info.name = item.value("name", "Unknown");
					info.flag = item.value("flag", "");

					std::string exeName = item.value("executable", "");
					info.executablePath = (cliPath.parent_path() / exeName).string();

					if (!info.flag.Empty() && !exeName.empty())
					{
						backends.push_back(info);
					}
				}
			}
		}
		catch (const std::exception& e)
		{
			throw std::runtime_error("Error parsing backends.json: " + std::string(e.what()));
		}

		return backends;
	}
};

} // namespace igni::backend