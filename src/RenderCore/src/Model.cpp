
#include <RenderCore/Model.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <iostream>
#include <unordered_map>

namespace re
{

const std::vector<Vertex>& Model::Vertices() const
{
	return m_vertices;
}

const std::vector<uint32_t>& Model::Indices() const
{
	return m_indices;
}

bool Model::LoadFromFile(String const& filePath)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	// Загружаем файл
	const bool success = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filePath.ToString().c_str());

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

	std::unordered_map<Vertex, std::uint32_t> uniqueVertices;

	// Проходим по всем формам (shapes) в .obj файле
	for (const auto& shape : shapes)
	{
		// Проходим по всем индексам формы
		for (const auto& index : shape.mesh.indices)
		{
			Vertex vertex{};

			// 1. Позиция
			vertex.position = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2],
			};

			// 2. Нормаль (если есть в файле)
			if (index.normal_index >= 0)
			{
				vertex.normal = {
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2]
				};
			}

			// 3. Текстурные координаты (если есть в файле)
			if (index.texcoord_index >= 0)
			{
				vertex.texCoord = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					// В формате OBJ ось V инвертирована относительно OpenGL, поэтому 1.0f - V
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
				};
			}

			vertex.color = Color::Gray;

			// 4. Дедупликация (формирование единого индексного буфера)
			if (!uniqueVertices.contains(vertex))
			{
				uniqueVertices[vertex] = static_cast<std::uint32_t>(m_vertices.size());
				m_vertices.push_back(vertex);
			}

			m_indices.push_back(uniqueVertices[vertex]);
		}
	}

	std::cout << "[Model Loaded]: " << filePath << " | Vertices: " << m_vertices.size() << " | Indices: " << m_indices.size() << std::endl;

	return true;
}

} // namespace re