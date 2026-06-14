#pragma once

#include <Core/Process.hpp>
#include <Core/String.hpp>

#include <nlohmann/json.hpp>

#include <unordered_map>
#include <unordered_set>

namespace igni
{

struct TargetConfig
{
	re::String prettyName;

	std::unordered_map<re::String, re::String> primitiveMapping;

	std::unordered_set<re::String> ffiAnnotations;

	static TargetConfig LoadFromBackend(const re::String& executablePath)
	{
		const auto command = executablePath + " --config";

		auto [exitCode, output] = re::Process::Run(command);

		if (exitCode != 0)
		{
			throw std::runtime_error("Backend initialization failed with code " + std::to_string(exitCode) + ". Output:\n" + output);
		}

		TargetConfig config;
		try
		{
			auto json = nlohmann::json::parse(output);

			if (json.contains("name"))
			{
				config.prettyName = json["name"].get<std::string>();
			}

			if (json.contains("primitiveMapping"))
			{
				for (const auto& [igniType, nativeType] : json["primitiveMapping"].items())
				{
					config.primitiveMapping[igniType.c_str()] = nativeType.get<std::string>().c_str();
				}
			}

			if (json.contains("ffiAnnotations"))
			{
				for (const auto& anno : json["ffiAnnotations"])
				{
					config.ffiAnnotations.emplace(anno.get<std::string>().c_str());
				}
			}
		}
		catch (const std::exception& e)
		{
			throw std::runtime_error("Failed to parse backend config JSON: " + std::string(e.what()));
		}

		return config;
	}
};

} // namespace igni