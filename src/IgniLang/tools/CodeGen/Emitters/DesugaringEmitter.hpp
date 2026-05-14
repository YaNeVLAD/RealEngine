#pragma once

#include "../Builder.hpp"

#include <Core/HashedString.hpp>

#include <nlohmann/json.hpp>

namespace igni::codegen
{
using namespace re::literals;

class DesugaringEmitter
{
public:
	static constexpr auto TargetKey = "operator_desugaring"_hs;

	void Emit(const nlohmann::json& data, Builder<std::string>& builder)
	{
		auto funcScope = builder.Function("re::String", "GetDesugaredMethod",
			"const re::String& astNodeName, bool isWrite");

		for (auto& [nodeName, rules] : data.items())
		{
			auto ifScope = builder.If("astNodeName == \"" + nodeName + "\"");

			builder.Line("if (isWrite) return \"" + rules["write"].get<std::string>() + "\";");
			builder.Line("return \"" + rules["read"].get<std::string>() + "\";");
		}

		builder.EmptyLine();
		builder.Line("return \"\";");
	}
};

} // namespace igni::codegen