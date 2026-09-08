#include <Core/LibraryLoader.hpp>

namespace re
{

template <typename T>
T LibraryLoader::GetSymbol(String const& name) const
{
	auto expected = GetSymbolAddress(name);
	if (!expected)
	{
		throw std::runtime_error(expected.error());
	}

	return reinterpret_cast<T>(*expected);
}

template <typename T>
std::expected<T, String> LibraryLoader::TryGetSymbol(String const& name) const noexcept
{
	auto expected = GetSymbolAddress(name);
	if (!expected)
	{
		return std::unexpected(expected.error());
	}

	return reinterpret_cast<T>(*expected);
}

} // namespace re