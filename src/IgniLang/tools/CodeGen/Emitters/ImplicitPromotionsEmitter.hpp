#pragma once

#include "../Builder.hpp"

#include <Core/HashedString.hpp>

#include <nlohmann/json.hpp>

namespace igni::codegen
{
using namespace re::literals;

class ImplicitPromotionsEmitter
{
public:
	static constexpr auto TargetKey = "implicit_promotions"_hs;

	static void Emit(const nlohmann::json& data, Builder<std::string>& builder)
	{
		auto funcScope = builder.Function("bool", "IsImplicitlyConvertible",
			"const re::String& from, const re::String& to");

		for (const auto& rule : data)
		{
			auto from = rule["from"].get<std::string>();
			auto to = rule["to"].get<std::string>();

			std::string condition = "from == \"" + from + "\" && to == \"" + to + "\"";

			auto ifScope = builder.If(condition);
			builder.Line("return true;");
		}

		builder.EmptyLine();
		builder.Line("return false;");
	}
};

} // namespace igni::codegen