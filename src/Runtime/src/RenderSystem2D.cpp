#include <Runtime/RenderSystem2D.hpp>

#include <ECS/Scene/Scene.hpp>
#include <Render2D/Renderer2D.hpp>
#include <Runtime/Components.hpp>

namespace re::runtime
{

void RenderSystem2D::Update(ecs::Scene& scene, float dt)
{
	core::Vector2f cameraPos = { 0, 0 };
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

	for (auto&& [_, transform, sprite] : *scene.CreateView<TransformComponent, SpriteComponent>())
	{
		core::Vector2f size = {
			sprite.size.x * transform.scale.x,
			sprite.size.y * transform.scale.y
		};

		render::Renderer2D::DrawQuad(transform.position, size, sprite.color);
	}

	render::Renderer2D::EndScene();
}

} // namespace re::runtime