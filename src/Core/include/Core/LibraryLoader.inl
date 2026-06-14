#include <Core/LibraryLoader.hpp>

namespace re
{

template <typename T>
T LibraryLoader::GetSymbol(String const& name) const
{
	auto symbol = reinterpret_cast<void*>(GetProcAddress(m_handle, name.ToString().c_str()));
	if (!symbol)
	{
		throw std::runtime_error("Failed to find symbol: " + name);
	}

	return reinterpret_cast<T>(symbol);
}

} // namespace re