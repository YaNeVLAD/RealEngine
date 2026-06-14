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
	void Run(const nlohmann::json& root, Builder<std::string>& headerBuilder, Builder<std::string>& sourceBuilder) const
	{
		(RunEmitter<TEmitters>(root, headerBuilder, sourceBuilder), ...);
	}

private:
	template <typename TEmitter>
	void RunEmitter(const nlohmann::json& root, Builder<std::string>& headerBuilder, Builder<std::string>& sourceBuilder) const
	{
		if (const char* key = TEmitter::TargetKey; root.contains(key))
		{
			TEmitter emitter;
			emitter.Emit(root[key], headerBuilder, sourceBuilder);

			headerBuilder.EmptyLine();
			sourceBuilder.EmptyLine();
		}
		else
		{
			std::cerr << "[Warning] JSON key not found for emitter: " << key << "\n";
		}
	}
};

} // namespace igni::codegen