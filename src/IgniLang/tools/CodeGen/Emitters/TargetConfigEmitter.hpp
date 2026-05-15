#pragma once

#include "../Builder.hpp"

#include <Core/HashedString.hpp>

#include <nlohmann/json.hpp>

namespace igni::codegen
{

using namespace re::literals;

class TargetConfigEmitter
{
public:
	static constexpr auto TargetKey = "targets"_hs;

	static void Emit(const nlohmann::json& data, Builder<std::string>& h, Builder<std::string>& cpp)
	{
		h.Line("struct TargetConfig {");
		h.Line("    re::String targetId;");
		h.Line("    std::unordered_map<re::String, re::String> primitiveMapping;");
		h.Line("    std::unordered_set<re::String> ffiAnnotations;");
		h.Line("};");
		h.EmptyLine();
		h.Line("TargetConfig GetTargetConfig(const re::String& targetId);");

		auto funcScope = cpp.Function("TargetConfig", "GetTargetConfig", "const re::String& targetId");
		for (auto it = data.begin(); it != data.end(); ++it)
		{
			const std::string& name = it.key();
			auto ifScope = cpp.If("targetId == \"" + name + "\"");
			cpp.Line("TargetConfig config;");
			cpp.Line("config.targetId = \"" + name + "\";");
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