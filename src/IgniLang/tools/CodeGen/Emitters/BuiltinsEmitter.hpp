#pragma once

#include <string>

#include "../Builder.hpp"

#include <Core/HashedString.hpp>

#include <nlohmann/json.hpp>

namespace igni::codegen
{
using namespace re::literals;

class BuiltinsEmitter
{
public:
	static constexpr auto TargetKey = "intrinsics"_hs;

	static void Emit(const nlohmann::json& data, Builder<std::string>& builder)
	{
		auto funcScope = builder.Function("bool", "TryMatchIntrinsic",
			"const re::String& name, std::size_t argsCount, bool hasTypeArgs, igni::CallDispatchType& outDispatch, re::String& outRetType");

		for (const auto& intr : data)
		{
			auto name = intr["name"].get<std::string>();
			auto dispatch = intr["dispatch"].get<std::string>();
			auto ret = intr["returns_type"].get<std::string>();
			auto args = intr["expected_args"].get<int>();
			auto hasTypeArgs = intr["requires_type_args"].get<bool>();

			std::string condition = "name == \"" + name + "\" && argsCount == " + std::to_string(args) + " && hasTypeArgs == " + (hasTypeArgs ? "true" : "false");

			auto ifScope = builder.If(condition);
			builder.Line("outDispatch = igni::CallDispatchType::" + dispatch + ";");
			builder.Line("outRetType = \"" + ret + "\";");
			builder.Line("return true;");
		}

		builder.Line("return false;");
	}
};

} // namespace igni::codegen