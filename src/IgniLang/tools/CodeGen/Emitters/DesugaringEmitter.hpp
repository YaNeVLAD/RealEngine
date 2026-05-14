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

	static void Emit(const nlohmann::json& data, Builder<std::string>& h, Builder<std::string>& cpp)
	{
		h.Line("re::String GetDesugaredMethod(const re::String& astNodeName, bool isWrite);");
		h.EmptyLine();

		auto funcScope = cpp.Function("re::String", "GetDesugaredMethod",
			"const re::String& astNodeName, bool isWrite");

		for (auto& [nodeName, rules] : data.items())
		{
			auto ifScope = cpp.If("astNodeName == \"" + nodeName + "\"");

			cpp.Line("if (isWrite) return \"" + rules["write"].get<std::string>() + "\";");
			cpp.Line("return \"" + rules["read"].get<std::string>() + "\";");
		}

		cpp.EmptyLine();
		cpp.Line("return \"\";");
	}
};

} // namespace igni::codegen