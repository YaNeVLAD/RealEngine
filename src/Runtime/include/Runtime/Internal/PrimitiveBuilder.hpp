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

		/*
			Front Face, Z = 0.5:
			(y)
			 ^
			 |  3----------2  (0.5, 0.5)
			 |  |          |
			 |  |    +Z    |
			 |  |          |
			 |  0----------1  (0.5, -0.5)
			 +---------------------------> (x)
		  (-0.5, -0.5)
		*/

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

	static std::pair<std::vector<Vertex>, std::vector<std::uint32_t>> CreateSphere(
		const Color& color,
		const float radius = 0.5f,
		const std::uint32_t sectorCount = 36,
		const std::uint32_t stackCount = 18)
	{
		std::vector<Vertex> vertices;
		std::vector<std::uint32_t> indices;

		vertices.reserve((stackCount + 1) * (sectorCount + 1));
		indices.reserve(stackCount * sectorCount * 6);

		const float pi = std::acos(-1.0f);
		const float sectorStep = 2.0f * pi / static_cast<float>(sectorCount);
		const float stackStep = pi / static_cast<float>(stackCount);
		const float lengthInv = 1.0f / radius;

		for (std::uint32_t i = 0; i <= stackCount; ++i)
		{
			const float stackAngle = pi / 2.0f - static_cast<float>(i) * stackStep;
			const float xy = radius * std::cos(stackAngle);
			const float z = radius * std::sin(stackAngle);

			for (std::uint32_t j = 0; j <= sectorCount; ++j)
			{
				const float sectorAngle = static_cast<float>(j) * sectorStep;

				const float x = xy * std::cos(sectorAngle);
				const float y = xy * std::sin(sectorAngle);

				const float nx = x * lengthInv;
				const float ny = y * lengthInv;
				const float nz = z * lengthInv;

				const float u = static_cast<float>(j) / static_cast<float>(sectorCount);
				const float v = static_cast<float>(i) / static_cast<float>(stackCount);

				Vertex vertex;
				vertex.position = { x, y, z };
				vertex.normal = { nx, nz, ny };
				vertex.color = color;
				vertex.texCoord = { u, v };
				vertex.texIndex = -1.f;
				vertex.position = { x, z, y };

				vertices.push_back(vertex);
			}
		}

		for (std::uint32_t i = 0; i < stackCount; ++i)
		{
			std::uint32_t k1 = i * (sectorCount + 1);
			std::uint32_t k2 = k1 + sectorCount + 1;

			for (std::uint32_t j = 0; j < sectorCount; ++j, ++k1, ++k2)
			{
				if (i != 0)
				{
					indices.push_back(k1);
					indices.push_back(k1 + 1);
					indices.push_back(k2);
				}

				if (i != (stackCount - 1))
				{
					indices.push_back(k1 + 1);
					indices.push_back(k2 + 1);
					indices.push_back(k2);
				}
			}
		}

		return { std::move(vertices), std::move(indices) };
	}

	static std::pair<std::vector<Vertex>, std::vector<std::uint32_t>> CreateOctahedron(const bool isWireframe = false)
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

			const Color finalColor = isWireframe ? Color(0, 0, 0, 255) : c;

			vertices.push_back({ p1, normal, finalColor, { 0.f, 0.f }, -1.f });
			vertices.push_back({ p2, normal, finalColor, { 1.f, 0.f }, -1.f });
			vertices.push_back({ p3, normal, finalColor, { 0.f, 1.f }, -1.f });

			indices.push_back(indexOffset + 0);
			indices.push_back(indexOffset + 1);
			indices.push_back(indexOffset + 2);
			indexOffset += 3;
		};

		constexpr std::uint8_t alpha = 215;
		addFace(left, front, top, Color(85, 160, 75, alpha));
		addFace(front, right, top, Color(210, 140, 70, alpha));
		addFace(front, left, bottom, Color(45, 110, 65, alpha));
		addFace(right, front, bottom, Color(190, 150, 170, alpha));

		addFace(back, left, top, Color(40, 90, 40, alpha));
		addFace(right, back, top, Color(150, 90, 40, alpha));
		addFace(left, back, bottom, Color(30, 70, 40, alpha));
		addFace(back, right, bottom, Color(130, 90, 100, alpha));

		return { std::move(vertices), std::move(indices) };
	}

	static std::pair<std::vector<Vertex>, std::vector<uint32_t>> CreateQuad3D(const Color& color)
	{
		std::vector<Vertex> vertices = {
			{ { -1.f, -1.f, 0.f }, { 0.f, 0.f, 1.f }, color, { 0.f, 0.f }, -1.f },
			{ { 1.f, -1.f, 0.f }, { 0.f, 0.f, 1.f }, color, { 1.f, 0.f }, -1.f },
			{ { 1.f, 1.f, 0.f }, { 0.f, 0.f, 1.f }, color, { 1.f, 1.f }, -1.f },
			{ { -1.f, 1.f, 0.f }, { 0.f, 0.f, 1.f }, color, { 0.f, 1.f }, -1.f }
		};
		std::vector<uint32_t> indices = { 0, 1, 2, 2, 3, 0 };
		return { std::move(vertices), std::move(indices) };
	}
};

} // namespace re::detail
