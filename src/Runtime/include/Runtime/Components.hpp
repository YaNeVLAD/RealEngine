#pragma once

#include <Core/Math/Color.hpp>
#include <Core/Math/Vector2.hpp>
#include <Core/Math/Vector3.hpp>
#include <Core/String.hpp>
#include <ECS/Scene/Scene.hpp>
#include <RenderCore/Font.hpp>
#include <RenderCore/Image.hpp>
#include <RenderCore/PrimitiveType.hpp>
#include <RenderCore/StaticMesh.hpp>
#include <RenderCore/Texture.hpp>
#include <RenderCore/Vertex.hpp>

#include <compare>

namespace re
{

struct TransformComponent
{
	Vector3f position = { 0.f, 0.f, 0.f };
	Vector3f rotation = { 0.f, 0.f, 0.f };
	Vector3f scale = { 1.f, 1.f, 1.f };

	glm::mat4 modelMatrix = glm::mat4(1.0f);
};

struct CircleComponent
{
	Color color = Color::White;
	float radius = 50.f;
};

struct RectangleComponent
{
	Color color = Color::White;
	Vector2f size = { 50.f, 50.f };
};

struct TextComponent
{
	String text;
	std::shared_ptr<Font> font;
	Color color = Color::White;
	float size = 24.f;
};

struct DynamicTextureComponent
{
	Image image;
	std::shared_ptr<Texture> texture = nullptr;
	bool isDirty = true;
};

struct MeshComponent
{
	std::vector<Vertex> vertices;
	PrimitiveType type = PrimitiveType::Triangles;
};

struct CameraComponent
{
	bool isPrimal = true;

	float fov = 45.f;
	float nearClip = 0.1f;
	float farClip = 1000.f;

	float zoom = 1.f;

	Vector2f mouseDelta = { 0.f, 0.f };
	Vector3f up = { 0.f, 1.f, 0.f };
};

struct BoxColliderComponent
{
	Vector2f size;
	Vector2f position;

	[[nodiscard]] bool Contains(const Vector2f point) const
	{
		const float dx = std::abs(point.x - position.x);
		const float dy = std::abs(point.y - position.y);

		return dx <= size.x * 0.5f && dy <= size.y * 0.5f;
	}

	[[nodiscard]] bool Intersects(BoxColliderComponent const& rhs) const
	{
		const float dx = std::abs(position.x - rhs.position.x);
		const float dy = std::abs(position.y - rhs.position.y);

		const float sumHalfWidths = (size.x + rhs.size.x) * 0.5f;
		const float sumHalfHeights = (size.y + rhs.size.y) * 0.5f;

		return dx <= sumHalfWidths && dy <= sumHalfHeights;
	}
};

struct DynamicMeshComponent3D
{
	std::vector<Vertex> vertices;
	std::vector<std::uint32_t> indices;
	bool wireframe = false;
};

struct StaticMeshComponent3D
{
	explicit StaticMeshComponent3D(const std::shared_ptr<StaticMesh>& mesh, const bool wireframe = false)
		: mesh(mesh)
		, wireframe(wireframe)
	{
	}

	StaticMeshComponent3D(std::vector<Vertex> vertices, std::vector<std::uint32_t> indices, const bool wireframe = false)
		: mesh(std::make_shared<StaticMesh>(vertices, indices))
		, wireframe(wireframe)
	{
	}

	std::shared_ptr<StaticMesh> mesh;
	bool wireframe;
};

struct DirectionalLightComponent
{
	Color color = Color::White;
	float ambientIntensity = 0.3f;
};

template <typename T>
using Dirty = detail::DirtyTag<T>;

} // namespace re
