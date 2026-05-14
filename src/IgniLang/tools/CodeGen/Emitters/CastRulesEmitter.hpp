#pragma once

#include "../Builder.hpp"

#include <Core/HashedString.hpp>

#include <nlohmann/json.hpp>

namespace igni::codegen
{

using namespace re::literals;

class CastRulesEmitter
{
public:
	static constexpr auto TargetKey = "valid_primitive_casts"_hs;

	static void Emit(const nlohmann::json& data, Builder<std::string>& builder)
	{
		auto funcScope = builder.Function("bool", "IsValidPrimitiveCast",
			"const re::String& from, const re::String& to");

		for (const auto& rule : data)
		{
			std::string from = rule["from"].get<std::string>();
			std::string to = rule["to"].get<std::string>();

			std::string condition = "from == \"" + from + "\" && to == \"" + to + "\"";

			auto ifScope = builder.If(condition);
			builder.Line("return true;");
		}

		builder.EmptyLine();
		builder.Line("return false;");
	}
};

} // namespace igni::codegen