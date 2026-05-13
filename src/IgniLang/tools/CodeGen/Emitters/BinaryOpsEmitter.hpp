#pragma once

#include "../Builder.hpp"

#include <Core/HashedString.hpp>

#include <nlohmann/json.hpp>

namespace igni::codegen
{

using namespace re::literals;

class BinaryOpsEmitter
{
public:
	static constexpr auto TargetKey = "binary_operations"_hs;

	static void Emit(const nlohmann::json& data, Builder<std::string>& builder)
	{
		auto funcScope = builder.Function("re::String", "GetBinaryOperationResult",
			"const re::String& op, const re::String& lhs, const re::String& rhs");

		for (const auto& rule : data)
		{
			auto opHash = rule["op"].get<std::string>();
			auto lhs = rule["lhs"].get<std::string>();
			auto rhs = rule["rhs"].get<std::string>();
			auto result = rule["result"].get<std::string>();

			std::string condition = "op.Hashed() == \"" + opHash + "\"_hs && lhs == \"" + lhs + "\" && rhs == \"" + rhs + "\"";

			auto ifScope = builder.If(condition);
			builder.Line("return \"" + result + "\";");
		}

		builder.EmptyLine();
		builder.Line("return \"\";");
	}
};

} // namespace igni::codegen