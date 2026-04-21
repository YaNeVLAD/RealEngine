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
		255,
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

	auto loadTex = [&](const std::string& texName, const bool srgb) -> std::shared_ptr<Texture> {
		if (texName.empty())
		{
			return nullptr;
		}
		const String texPath = baseDir + texName;

		if (auto texture = std::make_shared<Texture>(); texture->LoadFromFileSRGB(texPath, srgb))
		{
			const_cast<AssetManager*>(manager)->Add(texPath, texture);

			std::cout << "  [Texture Loaded]: " << texPath << std::endl;

			return texture;
		}
		std::cerr << "  [Texture Failed]: " << texPath << std::endl;

		return nullptr;
	};

	for (const auto& mat : materials)
	{
		Material engineMat{
			.workflow = MaterialWorkflow::BlinnPhong,
			.albedoColor = TinyObjColorToColor(mat.diffuse),
			.emissionColor = TinyObjColorToColor(mat.emission),
			.specularColor = TinyObjColorToColor(mat.specular),
			.shininess = mat.shininess > 0.0f ? mat.shininess : 32.0f,
		};

		engineMat.albedoMap = loadTex(mat.diffuse_texname, true);
		engineMat.emissionMap = loadTex(mat.emissive_texname, true);

		std::string normalName = !mat.normal_texname.empty() ? mat.normal_texname : mat.bump_texname;
		engineMat.normalMap = loadTex(normalName, false);

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

				vertex.tangent = { 0.f, 0.f, 0.f, 1.f };

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
		if (part.material.normalMap && !part.indices.empty())
		{
			for (std::size_t i = 0; i < part.indices.size(); i += 3)
			{
				Vertex& v0 = part.vertices[part.indices[i]];
				Vertex& v1 = part.vertices[part.indices[i + 1]];
				Vertex& v2 = part.vertices[part.indices[i + 2]];

				auto p0 = v0.position;
				auto p1 = v1.position;
				auto p2 = v2.position;

				auto uv0 = v0.texCoord;
				auto uv1 = v1.texCoord;
				auto uv2 = v2.texCoord;

				auto edge1 = p1 - p0;
				auto edge2 = p2 - p0;
				auto deltaUV1 = uv1 - uv0;
				auto deltaUV2 = uv2 - uv0;

				float denominator = (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
				if (std::abs(denominator) > std::numeric_limits<float>::epsilon())
				{
					float f = 1.0f / denominator;
					Vector3f tangent(
						f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x),
						f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y),
						f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z));
					tangent.Normalize();

					v0.tangent = { v0.tangent.x + tangent.x, v0.tangent.y + tangent.y, v0.tangent.z + tangent.z, 1.0f };
					v1.tangent = { v1.tangent.x + tangent.x, v1.tangent.y + tangent.y, v1.tangent.z + tangent.z, 1.0f };
					v2.tangent = { v2.tangent.x + tangent.x, v2.tangent.y + tangent.y, v2.tangent.z + tangent.z, 1.0f };
				}
			}

			for (auto& v : part.vertices)
			{
				Vector3f t{ v.tangent.x, v.tangent.y, v.tangent.z };
				if (t.Length() > std::numeric_limits<float>::epsilon())
				{
					t.Normalize();
					v.tangent = { t, 1.0f };
				}
			}
		}

		if (!part.indices.empty())
		{
			m_parts.emplace_back(std::move(part));
		}
	}

	std::cout << "[Model Loaded]: " << filePath << " | Parts: " << m_parts.size() << std::endl;

	return true;
}

} // namespace re