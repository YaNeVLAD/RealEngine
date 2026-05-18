#pragma once

#include "../Builder.hpp"

#include <Core/FlatMap.hpp>
#include <Core/HashedString.hpp>

#include <nlohmann/json.hpp>

namespace igni::codegen
{

using namespace re::literals;

class TargetConfigEmitter
{
	template <std::size_t N>
	using HashedStringMap = re::FlatMap<re::HashedString, std::string_view, N>;

public:
	static constexpr auto TargetKey = "targets"_hs;

	static void Emit(const nlohmann::json& data, Builder<std::string>& h, Builder<std::string>& cpp)
	{
		static constexpr HashedStringMap BUILD_TARGET_MAP = { {
			{ "rvm"_hs, "BuildTarget::RVM" },
			{ "dotnet"_hs, "BuildTarget::DotNet" },
		} };

		h.Line("struct TargetConfig {");
		h.Line("    BuildTarget target;");
		h.Line("    std::unordered_map<re::String, re::String> primitiveMapping;");
		h.Line("    std::unordered_set<re::String> ffiAnnotations;");
		h.Line("};");
		h.EmptyLine();
		h.Line("TargetConfig GetTargetConfig(BuildTarget target);");

		auto funcScope = cpp.Function("TargetConfig", "GetTargetConfig", "const BuildTarget target");
		for (auto it = data.begin(); it != data.end(); ++it)
		{
			const auto& key = it.key();
			const auto name = std::string(*BUILD_TARGET_MAP[re::HashedString(key)]);
			auto ifScope = cpp.If("target == " + name);
			cpp.Line("TargetConfig config;");
			cpp.Line("config.target = " + name + ";");
			for (auto& [k, v] : it.value()["primitive_mapping"].items())
			{
				cpp.Line("config.primitiveMapping[\"" + k + "\"] = \"" + v.get<std::string>() + "\";");
			}

			if (it.value().contains("ffi_annotations"))
			{
				for (auto& annotation : it.value()["ffi_annotations"])
				{
					cpp.Line("config.ffiAnnotations.insert(\"" + annotation.get<std::string>() + "\");");
				}
			}

			cpp.Line("return config;");
		}
		cpp.Line("return {};");
	}
};

} // namespace igni::codegen