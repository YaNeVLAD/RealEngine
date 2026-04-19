#pragma once

#include <RenderCore/Export.hpp>

#include <RenderCore/Assets/IAsset.hpp>

#include <cstdint>

namespace re
{

class RE_RENDER_CORE_API Texture final : public IAsset
{
public:
	Texture() = default;
	Texture(std::uint32_t width, std::uint32_t height);
	~Texture() override;

	void SetData(const std::uint8_t* data, std::uint32_t size);

	[[nodiscard]] std::uint32_t Width() const;
	[[nodiscard]] std::uint32_t Height() const;
	[[nodiscard]] std::uint32_t Size() const;

	[[nodiscard]] void* GetNativeHandle();

	bool LoadFromMemory(const std::uint8_t* data, std::size_t size);

	bool LoadFromFile(String const& filePath, const AssetManager* manager) override;

private:
	std::uint32_t m_width{};
	std::uint32_t m_height{};

	std::uint32_t m_rendererID = 0;
};

} // namespace re