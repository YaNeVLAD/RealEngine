#pragma once

#include <Runtime/Export.hpp>

#include <ECS/System/System.hpp>
#include <RenderCore/Font.hpp>
#include <RenderCore/IWindow.hpp>
#include <RenderCore/PrimitiveType.hpp>
#include <RenderCore/Texture.hpp>
#include <RenderCore/Vertex.hpp>

namespace re::detail
{

enum class RenderCmdType
{
	Quad,
	Circle,
	TexturedQuad,
	Text,
	Mesh
};

struct RenderCommand
{
	float z = 0.f;
	RenderCmdType type = RenderCmdType::Quad;

	Vector3f position = { 0.f, 0.f, 0.f };
	Vector2f size = { 0.f, 0.f };
	float rotation = 0.f;
	Color color = Color::White;

	Texture* texture = nullptr;
	Font* font = nullptr;
	String text = {};

	std::vector<Vertex> meshVertices = {};
	PrimitiveType primitiveType = PrimitiveType::Points;
};

class RE_RUNTIME_API RenderSystem2D final : public ecs::System
{
public:
	explicit RenderSystem2D(render::IWindow& window);

	void Update(ecs::Scene& scene, core::TimeDelta dt) override;

private:
	render::IWindow& m_window;
	std::vector<RenderCommand> m_renderQueue;
};

} // namespace re::detail