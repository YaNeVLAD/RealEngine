#pragma once

#include <RenderCore/Export.hpp>

#include <Core/Assets/IAsset.hpp>
#include <RenderCore/Material.hpp>
#include <RenderCore/Vertex.hpp>

#include <vector>

namespace re
{

struct MeshPart
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	Material material;
};

class RE_RENDER_CORE_API Model final : public IAsset
{
public:
	[[nodiscard]] const std::vector<MeshPart>& GetParts() const;

	bool LoadFromFile(String const& filePath) override;

private:
	std::vector<MeshPart> m_parts;
};

} // namespace re