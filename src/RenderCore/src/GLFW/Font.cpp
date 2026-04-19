#include <RenderCore/GLFW/Font.hpp>

#include <iostream>

namespace re
{

bool Font::LoadFromFile(String const& filePath, const AssetManager*)
{
	std::cout << "[WARNING] Font loading not implemented in OpenGL yet: " << filePath.ToString() << std::endl;

	return true;
}

void* Font::GetNativeHandle() const
{
	return nullptr;
}

} // namespace re