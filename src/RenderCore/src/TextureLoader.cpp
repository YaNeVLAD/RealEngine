#include <RenderCore/LoadTexture.hpp>

#include <RenderCore/Image.hpp>

#include <algorithm>
#include <cstring>
#include <expected>
#include <fstream>
#include <iostream>

namespace
{

namespace dds
{

constexpr auto EXT = ".dds";
constexpr auto SIGNATURE = "DDS ";
constexpr auto SIGNATURE_LEN = 4;

// Размеры заголовков
constexpr std::streamsize HEADER_SIZE = 128; // Magic (4) + Header (124)

// Форматы сжатия (FourCC)
constexpr std::uint32_t FOURCC_DXT1 = 0x31545844;
constexpr std::uint32_t FOURCC_DXT3 = 0x33545844;
constexpr std::uint32_t FOURCC_DXT5 = 0x35545844;

// Смещения байтов в заголовке DDS (относительно начала массива header)
namespace offset
{
constexpr std::size_t Height = 8;
constexpr std::size_t Width = 12;
constexpr std::size_t MipMapCount = 24;
constexpr std::size_t FourCC = 80;
} // namespace offset

std::uint32_t ReadUInt32(const std::uint8_t* buffer, const std::size_t offset)
{
	return *reinterpret_cast<const std::uint32_t*>(buffer + offset);
}

}; // namespace dds

using ParsingResult = std::expected<re::render::TextureData, std::string>;

ParsingResult ParseDDS(const std::string& path)
{
	using namespace re::render;

	TextureData result;

	std::ifstream file(path, std::ios::binary);
	if (!file)
	{
		return std::unexpected{ "[TextureParser]: Failed to open DDS file: " + path };
	}

	char magic[dds::SIGNATURE_LEN];
	file.read(magic, dds::SIGNATURE_LEN);
	if (strncmp(magic, dds::SIGNATURE, dds::SIGNATURE_LEN) != 0)
	{
		return std::unexpected{ "[TextureParser]: Not a valid DDS file: " + path };
	}

	unsigned char header[dds::HEADER_SIZE - dds::SIGNATURE_LEN];
	file.read(reinterpret_cast<char*>(header), dds::HEADER_SIZE - dds::SIGNATURE_LEN);

	result.height = dds::ReadUInt32(header, dds::offset::Height);
	result.width = dds::ReadUInt32(header, dds::offset::Width);
	result.mipMapCount = dds::ReadUInt32(header, dds::offset::MipMapCount);
	const auto fourCC = dds::ReadUInt32(header, dds::offset::FourCC);

	// clang-format off
	switch (fourCC)
	{
	case dds::FOURCC_DXT1: result.format = TextureFormat::DXT1; break;
	case dds::FOURCC_DXT3: result.format = TextureFormat::DXT3; break;
	case dds::FOURCC_DXT5: result.format = TextureFormat::DXT5; break;
	default: return std::unexpected{ "[TextureParser]: Unsupported DDS format in " + path };
	}
	// clang-format on

	file.seekg(0, std::ios::end);
	const std::streamsize length = file.tellg();
	file.seekg(dds::HEADER_SIZE, std::ios::beg);

	result.buffer.resize(static_cast<std::size_t>(length) - dds::HEADER_SIZE);
	file.read(reinterpret_cast<char*>(result.buffer.data()), static_cast<std::streamsize>(result.buffer.size()));

	return result;
}

ParsingResult ParseStandard(const std::string& path)
{
	using namespace re::render;

	TextureData result;

	re::Image tempImage;
	if (!tempImage.LoadFromFile(path, nullptr))
	{
		return std::unexpected{ "[TextureParser]: Failed to load standard image: " + path };
	}

	result.width = tempImage.Width();
	result.height = tempImage.Height();
	result.mipMapCount = 1;
	result.format = (tempImage.Channels() == 3) ? TextureFormat::RGB : TextureFormat::RGBA;

	const std::size_t sizeBytes = tempImage.Width() * tempImage.Height() * tempImage.Channels();
	result.buffer.assign(tempImage.Data(), tempImage.Data() + sizeBytes);

	return result;
}

template <typename TFn>
	requires std::invocable<TFn, std::string const&> && std::same_as<std::invoke_result_t<TFn, std::string const&>, ParsingResult>
std::optional<re::render::TextureData> ParseAndPrintOnError(TFn&& fn, std::string const& path)
{
	const ParsingResult result = std::invoke(std::forward<TFn>(fn), path);
	if (!result.has_value())
	{
		std::cerr << result.error() << std::endl;
	}

	if (result.has_value())
	{
		return result.value();
	}
	return std::nullopt;
}

} // namespace

namespace re::render
{

std::optional<TextureData> LoadTexture(String const& filePath)
{
	const std::string pathStr = filePath.ToString();
	std::string lowerPath = pathStr;
	std::ranges::transform(lowerPath, lowerPath.begin(), [](const char ch) {
		return static_cast<char>(std::tolower(ch));
	});

	if (lowerPath.length() >= 4 && lowerPath.substr(lowerPath.length() - 4) == dds::EXT)
	{
		return ParseAndPrintOnError(ParseDDS, pathStr);
	}

	return ParseAndPrintOnError(ParseStandard, pathStr);
}

} // namespace re::render