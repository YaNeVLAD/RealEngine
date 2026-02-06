#include <Runtime/Internal/RenderSystem2D.hpp>

#include <ECS/Scene/Scene.hpp>
#include <Render2D/Renderer2D.hpp>
#include <Runtime/Components.hpp>

namespace re::detail
{

RenderSystem2D::RenderSystem2D(render::IWindow& window)
	: m_window(window)
{
}

void RenderSystem2D::Update(ecs::Scene& scene, core::TimeDelta)
{
	Vector2f cameraPos = { 0, 0 };
	float cameraZoom = 1.0f;

	for (auto&& [_, transform, camera] : *scene.CreateView<TransformComponent, CameraComponent>())
	{
		if (camera.isPrimal)
		{
			cameraPos = transform.position;
			cameraZoom = camera.zoom;
			break;
		}
	}

	render::Renderer2D::BeginScene(cameraPos, cameraZoom);

	for (auto&& [_, transform, sprite] : *scene.CreateView<TransformComponent, RectangleComponent>())
	{
		Vector2f size = {
			sprite.size.x * transform.scale.x,
			sprite.size.y * transform.scale.y
		};

		render::Renderer2D::DrawQuad(
			transform.position,
			size,
			transform.rotation,
			sprite.color);
	}

	for (auto&& [_, transform, circle] : *scene.CreateView<TransformComponent, CircleComponent>())
	{
		const float radius = circle.radius * transform.scale.x;

		render::Renderer2D::DrawCircle(
			transform.position,
			radius,
			circle.color);
	}

	render::Renderer2D::EndScene();
}

} // namespace re::detail