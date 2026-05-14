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

	static void Emit(const nlohmann::json& data, Builder<std::string>& h, Builder<std::string>& cpp)
	{
		h.Line("bool TryMatchIntrinsic(const re::String& name, std::size_t argsCount, bool hasTypeArgs, igni::CallDispatchType& outDispatch, re::String& outRetType);");
		h.EmptyLine();

		auto funcScope = cpp.Function("bool", "TryMatchIntrinsic",
			"const re::String& name, std::size_t argsCount, bool hasTypeArgs, igni::CallDispatchType& outDispatch, re::String& outRetType");

		for (const auto& intrinsic : data)
		{
			auto name = intrinsic["name"].get<std::string>();
			auto dispatch = intrinsic["dispatch"].get<std::string>();
			auto ret = intrinsic["returns_type"].get<std::string>();
			const auto args = intrinsic["expected_args"].get<int>();
			const auto hasTypeArgs = intrinsic["requires_type_args"].get<bool>();

			std::string condition = "name == \"" + name + "\" && argsCount == " + std::to_string(args) + " && hasTypeArgs == " + (hasTypeArgs ? "true" : "false");

			auto ifScope = cpp.If(condition);
			cpp.Line("outDispatch = igni::CallDispatchType::" + dispatch + ";");
			cpp.Line("outRetType = \"" + ret + "\";");
			cpp.Line("return true;");
		}

		cpp.Line("return false;");
	}
};

} // namespace igni::codegen