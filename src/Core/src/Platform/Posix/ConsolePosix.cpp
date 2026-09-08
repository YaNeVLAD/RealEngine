#include <Core/Console.hpp>

#include <clocale>

namespace re
{

void Console::SetupUTF8()
{
	std::setlocale(LC_ALL, "");
}

} // namespace re