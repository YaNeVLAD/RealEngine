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

	void Emit(const nlohmann::json& data, Builder<std::string>& builder) const
	{
		auto funcScope = builder.Function("re::String", "GetUnaryOperationResult",
			"const re::String& op, const re::String& operand");

		for (const auto& rule : data)
		{
			std::string opHash = rule["op"].get<std::string>();
			std::string operand = rule["operand"].get<std::string>();
			std::string result = rule["result"].get<std::string>();

			std::string condition = "op.Hashed() == \"" + opHash + "\"_hs && operand == \"" + operand + "\"";

			auto ifScope = builder.If(condition);
			builder.Line("return \"" + result + "\";");
		}

		builder.EmptyLine();
		builder.Line("return \"\";");
	}
};

} // namespace igni::codegen