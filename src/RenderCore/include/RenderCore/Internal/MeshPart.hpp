#pragma once

#include <RenderCore/GLFW/IndexBuffer.hpp>
#include <RenderCore/GLFW/VertexArray.hpp>
#include <RenderCore/GLFW/VertexBuffer.hpp>
#include <RenderCore/Material.hpp>
#include <RenderCore/Vertex.hpp>

#include <vector>

namespace re::render
{

struct MeshPart
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	Material material;

	std::shared_ptr<VertexArray> vao = nullptr;
	std::shared_ptr<VertexBuffer> vbo = nullptr;
	std::shared_ptr<IndexBuffer> ebo = nullptr;
};

} // namespace re::render