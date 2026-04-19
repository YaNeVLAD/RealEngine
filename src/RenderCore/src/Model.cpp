#include <RenderCore/Model.hpp>

#include "RenderCore/Assets/AssetManager.hpp"
#include <RenderCore/Material.hpp>
#include <RenderCore/Vertex.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <iostream>
#include <ranges>
#include <unordered_map>

namespace
{

re::Color TinyObjColorToColor(const tinyobj::real_t* color)
{
	return {
		static_cast<std::uint8_t>(color[0] * 255),
		static_cast<std::uint8_t>(color[1] * 255),
		static_cast<std::uint8_t>(color[2] * 255),
		static_cast<std::uint8_t>(color[3] * 255),
	};
}

} // namespace

namespace re
{

const std::vector<render::MeshPart>& Model::GetParts() const
{
	return m_parts;
}

bool Model::LoadFromFile(String const& filePath, const AssetManager* manager)
{
	m_parts.clear();

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	std::string filepathStr = filePath.ToString();

	std::string baseDir;
	if (std::size_t lastSlash = filepathStr.find_last_of("/\\"); lastSlash != std::string::npos)
	{
		baseDir = filepathStr.substr(0, lastSlash + 1);
	}

	const bool success = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepathStr.c_str(), baseDir.c_str());

	if (!warn.empty())
	{
		std::cout << "[Model Warning]: " << warn << std::endl;
	}
	if (!err.empty())
	{
		std::cerr << "[Model Error]: " << err << std::endl;
	}
	if (!success)
	{
		return false;
	}

	std::vector<Material> engineMaterials;
	engineMaterials.reserve(materials.size());

	for (const auto& mat : materials)
	{
		Material engineMat{
			.ambient = TinyObjColorToColor(mat.ambient),
			.diffuse = TinyObjColorToColor(mat.diffuse),
			.specular = TinyObjColorToColor(mat.specular),
			.emission = TinyObjColorToColor(mat.emission),
			.shininess = mat.shininess > 0.0f ? mat.shininess : 32.0f,
		};

		if (!mat.diffuse_texname.empty())
		{
			std::string texPath = baseDir + mat.diffuse_texname;
			if (auto texture = manager->Get<Texture>(String(texPath)))
			{
				engineMat.texture = texture;
				std::cout << "  [Texture Loaded]: " << texPath << std::endl;
			}
			else
			{
				std::cerr << "  [Texture Failed]: " << texPath << std::endl;
			}
		}
		engineMaterials.emplace_back(engineMat);
	}

	if (engineMaterials.empty())
	{
		engineMaterials.emplace_back();
	}

	std::unordered_map<int, render::MeshPart> partsMap;
	std::unordered_map<int, std::unordered_map<Vertex, std::uint32_t>> uniqueVerticesPerPart;

	for (const auto& shape : shapes)
	{
		std::size_t indexOffset = 0;

		for (std::size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++)
		{
			int matId = shape.mesh.material_ids[f];
			if (matId < 0 || matId >= engineMaterials.size())
			{
				matId = 0;
			}

			auto& [vertices, indices, material] = partsMap[matId];
			auto& uniqueVertices = uniqueVerticesPerPart[matId];

			material = engineMaterials[matId];

			auto fv = static_cast<size_t>(shape.mesh.num_face_vertices[f]);

			for (std::size_t v = 0; v < fv; v++)
			{
				auto [vertexIndex, normalIndex, texcoordIndex] = shape.mesh.indices[indexOffset + v];
				Vertex vertex{};

				vertex.position = {
					attrib.vertices[3 * vertexIndex + 0],
					attrib.vertices[3 * vertexIndex + 1],
					attrib.vertices[3 * vertexIndex + 2]
				};

				if (normalIndex >= 0)
				{
					vertex.normal = {
						attrib.normals[3 * normalIndex + 0],
						attrib.normals[3 * normalIndex + 1],
						attrib.normals[3 * normalIndex + 2]
					};
				}

				if (texcoordIndex >= 0)
				{
					vertex.texCoord = {
						attrib.texcoords[2 * texcoordIndex + 0],
						// In OBJ V axis is inverted comparing to OpenGL
						1.0f - attrib.texcoords[2 * texcoordIndex + 1],
					};
				}

				vertex.color = Color::White;

				if (!uniqueVertices.contains(vertex))
				{
					uniqueVertices[vertex] = static_cast<std::uint32_t>(vertices.size());
					vertices.push_back(vertex);
				}

				indices.push_back(uniqueVertices[vertex]);
			}
			indexOffset += fv;
		}
	}

	for (auto& part : partsMap | std::views::values)
	{
		if (!part.indices.empty())
		{
			m_parts.emplace_back(std::move(part));
		}
	}

	std::cout << "[Model Loaded]: " << filePath << " | Parts: " << m_parts.size() << std::endl;

	return true;
}

} // namespace re