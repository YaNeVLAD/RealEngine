#pragma once

#include <glm/glm.hpp>

namespace re::detail
{

class PrimitiveBuilder
{
public:
	static std::pair<std::vector<Vertex>, std::vector<std::uint32_t>> CreateCube(const Color& color)
	{
		std::vector<Vertex> vertices;
		vertices.reserve(24);

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

	static std::pair<std::vector<Vertex>, std::vector<std::uint32_t>> CreateOctahedron(const Color& color)
	{
		std::vector<Vertex> vertices;
		std::vector<std::uint32_t> indices;
		vertices.reserve(24);
		indices.reserve(24);

		constexpr Vector3f top = { 0.0f, 1.0f, 0.0f };
		constexpr Vector3f bottom = { 0.0f, -1.0f, 0.0f };
		constexpr Vector3f right = { 1.0f, 0.0f, 0.0f };
		constexpr Vector3f front = { 0.0f, 0.0f, 1.0f };
		constexpr Vector3f left = { -1.0f, 0.0f, 0.0f };
		constexpr Vector3f back = { 0.0f, 0.0f, -1.0f };

		std::uint32_t indexOffset = 0;

		auto addFace = [&](const Vector3f& p1, const Vector3f& p2, const Vector3f& p3, const Color& c) {
			const glm::vec3 u = { p2.x - p1.x, p2.y - p1.y, p2.z - p1.z };
			const glm::vec3 v = { p3.x - p1.x, p3.y - p1.y, p3.z - p1.z };
			const glm::vec3 n = glm::normalize(glm::cross(u, v));
			const Vector3f normal = { n.x, n.y, n.z };

			vertices.push_back({ p1, normal, c, { 0.f, 0.f }, -1.f });
			vertices.push_back({ p2, normal, c, { 1.f, 0.f }, -1.f });
			vertices.push_back({ p3, normal, c, { 0.f, 1.f }, -1.f });

			indices.push_back(indexOffset + 0);
			indices.push_back(indexOffset + 1);
			indices.push_back(indexOffset + 2);
			indexOffset += 3;
		};

		addFace(right, front, top, color);
		addFace(front, left, top, color);
		addFace(left, back, top, color);
		addFace(back, right, top, color);

		addFace(front, right, bottom, color);
		addFace(left, front, bottom, color);
		addFace(back, left, bottom, color);
		addFace(right, back, bottom, color);

		return { std::move(vertices), std::move(indices) };
	}
};

} // namespace re::detail