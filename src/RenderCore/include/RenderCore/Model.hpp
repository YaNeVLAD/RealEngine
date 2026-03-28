#pragma once

#include <RenderCore/Export.hpp>

#include <Core/Assets/IAsset.hpp>
#include <RenderCore/Vertex.hpp>

#include <vector>

namespace re
{

class RE_RENDER_CORE_API Model final : public IAsset
{
public:
	Model() = default;
	~Model() override = default;

	[[nodiscard]] const std::vector<Vertex>& Vertices() const;
	[[nodiscard]] const std::vector<uint32_t>& Indices() const;

	bool LoadFromFile(String const& filePath) override;

private:
	std::vector<Vertex> m_vertices;
	std::vector<uint32_t> m_indices;
};

} // namespace re