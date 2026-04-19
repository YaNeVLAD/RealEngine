#pragma once

#include <RenderCore/Export.hpp>

#include <RenderCore/Assets/IAsset.hpp>
#include <RenderCore/Internal/MeshPart.hpp>
#include <RenderCore/Vertex.hpp>

#include <vector>

namespace re
{

class RE_RENDER_CORE_API AnimatedModel final : public IAsset
{
public:
	[[nodiscard]] const std::vector<render::MeshPart>& GetParts() const;

	bool LoadFromFile(const String& filePath, const AssetManager* manager) override;

private:
	std::vector<render::MeshPart> m_parts;

	// TODO: Add:
	// std::vector<Bone> m_skeleton;
	// std::vector<Animation> m_animations;
};

} // namespace re