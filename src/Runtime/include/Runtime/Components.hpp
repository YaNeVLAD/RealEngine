#pragma once

#include <Core/Math/Color.hpp>
#include <Core/Math/Vector2.hpp>
#include <Core/Math/Vector3.hpp>
#include <Core/String.hpp>
#include <ECS/Scene.hpp>
#include <Physics/Types.hpp>
#include <RenderCore/Font.hpp>
#include <RenderCore/Image.hpp>
#include <RenderCore/Material.hpp>
#include <RenderCore/PrimitiveType.hpp>
#include <RenderCore/StaticMesh.hpp>
#include <RenderCore/Texture.hpp>
#include <RenderCore/Vertex.hpp>

#include <compare>
#include <utility>

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
	explicit StaticMeshComponent3D(std::shared_ptr<StaticMesh> const& mesh, const bool wireframe = false)
		: mesh(mesh)
		, wireframe(wireframe)
	{
	}

	StaticMeshComponent3D(const std::vector<Vertex>& vertices, const std::vector<std::uint32_t>& indices, const bool wireframe = false)
		: mesh(std::make_shared<StaticMesh>(vertices, indices))
		, wireframe(wireframe)
	{
	}

	std::shared_ptr<StaticMesh> mesh;
	bool wireframe;
};

enum class LightType : std::uint8_t
{
	Directional = 0,
	Point = 1,
	Spotlight = 2,
};

struct LightComponent
{
	LightType type = LightType::Directional;

	Color ambient = { 50, 50, 50, 255 };
	Color diffuse = { 255, 255, 255, 255 };
	Color specular = { 255, 255, 255, 255 };

	float constant = 1.0f;
	float linear = 0.09f;
	float quadratic = 0.032f;

	float cutOffAngle = 12.5f;
	float exponent = 1.0f;

	static LightComponent CreateDirectional(const Color diffuse = Color::White, const Color ambient = { 50, 50, 50, 255 }, const Color specular = Color::White)
	{
		LightComponent l;
		l.type = LightType::Directional;
		l.diffuse = diffuse;
		l.ambient = ambient;
		l.specular = specular;
		return l;
	}

	static LightComponent CreatePoint(const Color diffuse = Color::White, const float linearAtt = 0.007f, const float quadAtt = 0.0002f)
	{
		LightComponent l;
		l.type = LightType::Point;
		l.diffuse = diffuse;
		l.specular = diffuse;
		l.linear = linearAtt;
		l.quadratic = quadAtt;
		return l;
	}

	static LightComponent CreateSpotlight(const Color diffuse = Color::White, const float cutOffAngle = 12.5f, const float exponent = 1.0f)
	{
		LightComponent l;
		l.type = LightType::Spotlight;
		l.diffuse = diffuse;
		l.specular = diffuse;
		l.cutOffAngle = cutOffAngle;
		l.exponent = exponent;
		return l;
	}
};

struct MaterialComponent
{
	Material data;

	MaterialComponent() = default;

	explicit MaterialComponent(Material material)
		: data(std::move(material))
	{
	}

	explicit MaterialComponent([[maybe_unused]] const Color ambient, const Color diffuse, const Color specular, const Color emission, const float shininess = 32.f)
	{
		data.workflow = MaterialWorkflow::BlinnPhong;
		data.albedoColor = diffuse;
		data.specularColor = specular;
		data.emissionColor = emission;
		data.shininess = shininess;
	}

	bool operator==(MaterialComponent const&) const = default;

	bool operator<(MaterialComponent const& rhs) const
	{
		return data < rhs.data;
	}
};

class MouseLookSystem;
struct MouseLookComponent
{
	explicit MouseLookComponent(const float sensitivity = 0.1f)
		: sensitivity(sensitivity)
	{
	}

	float sensitivity = 0.1f;

private:
	static constexpr float MIN_PITCH = -89.0f;
	static constexpr float MAX_PITCH = 89.0f;

	bool firstMouse = true;
	float lastX = 0.0f;
	float lastY = 0.0f;
	float deltaX = 0.0f;
	float deltaY = 0.0f;

	friend class MouseLookSystem;
};

struct PhysicsEventsComponent
{
	std::vector<physics::CollisionPair> collisions;
};

using RigidBodyComponent = physics::RigidBody;

template <typename T>
using Dirty = detail::DirtyTag<T>;

} // namespace re
