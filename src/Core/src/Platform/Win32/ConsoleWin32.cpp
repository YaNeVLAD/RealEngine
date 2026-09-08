#include <Core/Console.hpp>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace re
{

void Console::SetupUTF8()
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
}

}