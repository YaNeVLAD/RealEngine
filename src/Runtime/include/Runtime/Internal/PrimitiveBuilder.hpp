#pragma once

#include <Runtime/Components.hpp>

#include <glm/glm.hpp>

namespace re::detail
{

class PrimitiveBuilder
{
public:
	// Возвращает готовые вершины и индексы для куба заданного цвета
	static std::pair<std::vector<Vertex>, std::vector<std::uint32_t>> CreateCube(const Color& color)
	{
		std::vector<Vertex> vertices;
		vertices.reserve(24);

		const auto [r, g, b, a] = color;
		const glm::vec4 c = { r / 255.f, g / 255.f, b / 255.f, a / 255.f };

		// Helper для быстрого добавления вершины
		auto addVertex = [&](float px, float py, float pz, float nx, float ny, float nz, float u, float v) {
			Vertex vertex;
			vertex.position = { px, py, pz };
			vertex.normal = { nx, ny, nz };
			vertex.color = color;
			vertex.texCoord = { u, v };
			vertices.push_back(vertex);
		};

		// Front face
		addVertex(-0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
		addVertex(0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
		addVertex(0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
		addVertex(-0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);

		// Back face
		addVertex(0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
		addVertex(-0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
		addVertex(-0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);
		addVertex(0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);

		// Left face
		addVertex(-0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
		addVertex(-0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
		addVertex(-0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
		addVertex(-0.5f, 0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

		// Right face
		addVertex(0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
		addVertex(0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
		addVertex(0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
		addVertex(0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

		// Top face
		addVertex(-0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
		addVertex(0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f);
		addVertex(0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f);
		addVertex(-0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f);

		// Bottom face
		addVertex(-0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f);
		addVertex(0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f);
		addVertex(0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f);
		addVertex(-0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f);

		std::vector<std::uint32_t> indices;
		indices.reserve(36);
		for (std::uint32_t i = 0; i < 6; ++i)
		{
			const std::uint32_t offset = i * 4;
			indices.push_back(offset + 0);
			indices.push_back(offset + 1);
			indices.push_back(offset + 2);
			indices.push_back(offset + 2);
			indices.push_back(offset + 3);
			indices.push_back(offset + 0);
		}

		return { std::move(vertices), std::move(indices) };
	}
};

} // namespace re::detail