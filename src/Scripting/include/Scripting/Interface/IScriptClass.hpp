#pragma once

#include <Core/String.hpp>
#include <Scripting/Interface/IScriptInstance.hpp>

#include <cstdint>
#include <memory>

namespace re::scripting
{

class IScriptClass
{
public:
	virtual ~IScriptClass() = default;

	[[nodiscard]] virtual String const& Name() const = 0;
	[[nodiscard]] virtual String const& Namespace() const = 0;

	virtual std::shared_ptr<IScriptInstance> Instantiate(std::uint64_t entityID) = 0;
};

} // namespace re::scripting
