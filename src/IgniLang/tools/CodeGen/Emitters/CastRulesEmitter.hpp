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

	static void Emit(const nlohmann::json& data, Builder<std::string>& h, Builder<std::string>& cpp)
	{
		h.Line("bool IsValidPrimitiveCast(const re::String& from, const re::String& to);");
		h.EmptyLine();

		auto funcScope = cpp.Function("bool", "IsValidPrimitiveCast",
			"const re::String& from, const re::String& to");

		for (const auto& rule : data)
		{
			auto from = rule["from"].get<std::string>();
			auto to = rule["to"].get<std::string>();

			std::string condition = "from == \"" + from + "\" && to == \"" + to + "\"";

			auto ifScope = cpp.If(condition);
			cpp.Line("return true;");
		}

		cpp.EmptyLine();
		cpp.Line("return false;");
	}
};

} // namespace igni::codegen