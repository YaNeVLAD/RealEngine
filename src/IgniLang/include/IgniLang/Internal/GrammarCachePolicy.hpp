#pragma once

#include <Core/String.hpp>

#include <fsm/cfg.hpp>
#include <fsm/lr/table_io.hpp>
#include <fsm/slr/table_builder.hpp>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

namespace igni::detail
{

struct GrammarCachePolicy
{
	static auto load(std::ifstream& cacheFile)
	{
		return fsm::lr::io::load_from_binary<re::String>(cacheFile);
	}

	[[nodiscard]] static auto build(const std::filesystem::path& sourcePath)
	{
		std::ifstream file(sourcePath);
		if (!file.is_open())
		{
			throw std::runtime_error("Failed to open grammar file: " + sourcePath.string());
		}

		const auto grammar = fsm::cfg_load<re::String>(file);
		fsm::slr::table_builder builder(grammar);
		auto table = builder
						 .with_epsilon("<EPSILON>")
						 .with_end_marker("<EOF>")
						 .build(fsm::slr::collision_policy::prefer_shift);

		return std::move(table);
	}

	template <typename TableType>
	static void save(const TableType& table, std::ofstream& cacheFile)
	{
		fsm::lr::io::save_to_binary(table, cacheFile);
	}
};

} // namespace igni::detail