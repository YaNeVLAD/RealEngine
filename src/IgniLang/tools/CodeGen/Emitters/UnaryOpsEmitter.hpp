#pragma once

#include "../Builder.hpp"

#include <Core/HashedString.hpp>

#include <nlohmann/json.hpp>

namespace igni::codegen
{

using namespace re::literals;

class UnaryOpsEmitter
{
public:
	static constexpr auto TargetKey = "unary_operations"_hs;

	static void Emit(const nlohmann::json& data, Builder<std::string>& builder)
	{
		auto funcScope = builder.Function("re::String", "GetUnaryOperationResult",
			"const re::String& op, const re::String& operand");

		for (const auto& rule : data)
		{
			auto opHash = rule["op"].get<std::string>();
			auto operand = rule["operand"].get<std::string>();
			auto result = rule["result"].get<std::string>();

			std::string condition = "op.Hashed() == \"" + opHash + "\"_hs && operand == \"" + operand + "\"";

			auto ifScope = builder.If(condition);
			builder.Line("return \"" + result + "\";");
		}

		builder.EmptyLine();
		builder.Line("return \"\";");
	}
};

} // namespace igni::codegen