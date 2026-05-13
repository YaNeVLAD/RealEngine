#pragma once

#include <iostream>

#include "Builder.hpp"

#include <nlohmann/json.hpp>

namespace igni::codegen
{

template <typename... TEmitters>
class GeneratorEngine
{
public:
	void Run(const nlohmann::json& root, Builder<std::string>& builder) const
	{
		(RunEmitter<TEmitters>(root, builder), ...);
	}

private:
	template <typename TEmitter>
	void RunEmitter(const nlohmann::json& root, Builder<std::string>& builder) const
	{
		if (const char* key = TEmitter::TargetKey; root.contains(key))
		{
			TEmitter emitter;
			emitter.Emit(root[key], builder);
			builder.EmptyLine();
		}
		else
		{
			std::cerr << "[Warning] JSON key not found for emitter: " << key << "\n";
		}
	}
};

} // namespace igni::codegen