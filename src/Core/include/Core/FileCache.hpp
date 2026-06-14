#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace re::FileCache
{

/**
 * Searches for source and cache files, compares their last write time and returns the result provided by Policy
 * @tparam Policy type with following methods:
 * - return_type load(std::ifstream& cacheIn)
 * - return_type build(const std::filesystem::path& source)
 * - void save(return_type, std::ofstream cacheOut)
 */
template <typename Policy>
auto Execute(
	const std::filesystem::path& sourcePath,
	const std::filesystem::path& cachePath,
	Policy&& policy = {})
{
	namespace fs = std::filesystem;
	bool loadFromCache = false;

	if (fs::exists(cachePath) && fs::exists(sourcePath))
	{
		if (fs::last_write_time(cachePath) >= fs::last_write_time(sourcePath))
		{
			loadFromCache = true;
		}
	}

	if (loadFromCache)
	{
		std::cout << "[Info] Loading from cache: " << cachePath.string() << "...\n";
		std::ifstream cacheFile(cachePath, std::ios::binary);
		if (!cacheFile.is_open())
		{
			throw std::runtime_error("Failed to open cache file: " + cachePath.string());
		}

		return policy.load(cacheFile);
	}

	std::cout << "[Info] Building from scratch (this may take a while)...\n";
	auto result = policy.build(sourcePath);

	std::cout << "[Info] Saving to cache...\n";
	std::ofstream cacheFile(cachePath, std::ios::binary);
	if (!cacheFile.is_open())
	{
		throw std::runtime_error("Failed to open cache file for writing: " + cachePath.string());
	}

	policy.save(result, cacheFile);

	return result;
}

} // namespace re::FileCache