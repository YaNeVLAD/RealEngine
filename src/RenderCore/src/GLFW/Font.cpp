#include <RenderCore/GLFW/Font.hpp>

#include <iostream>

namespace re
{

bool Font::LoadFromFile(std::string const& filePath)
{
	std::cout << "[WARNING] Font loading not implemented in OpenGL yet: " << filePath << std::endl;

	return true;
}

void* Font::GetNativeHandle() const
{
	return nullptr;
}

} // namespace re