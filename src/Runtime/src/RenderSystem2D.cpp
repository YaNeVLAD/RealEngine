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

	for (auto&& [_, transform, comp] : *scene.CreateView<TransformComponent, DynamicTextureComponent>())
	{
		if (comp.isDirty && !comp.image.IsEmpty())
		{
			if (!comp.texture
				|| comp.texture->Width() != comp.image.Width()
				|| comp.texture->Height() != comp.image.Height())
			{
				comp.texture = std::make_shared<re::Texture>(
					comp.image.Width(),
					comp.image.Height());
			}

			comp.texture->SetData(comp.image.Data(), comp.image.Size());

			comp.isDirty = false;
		}

		if (comp.texture)
		{
			Vector2f size = {
				static_cast<float>(comp.texture->Width()) * transform.scale.x,
				static_cast<float>(comp.texture->Height()) * transform.scale.y
			};

			render::Renderer2D::DrawTexturedQuad(
				transform.position,
				size,
				comp.texture.get(),
				Color::White);
		}
	}

	for (auto&& [_, transform, textComp] : *scene.CreateView<TransformComponent, TextComponent>())
	{
		if (textComp.font)
		{
			const float finalSize = textComp.size * transform.scale.x;

			render::Renderer2D::DrawText(
				textComp.text,
				*textComp.font,
				transform.position,
				finalSize,
				textComp.color);
		}
	}

	render::Renderer2D::EndScene();
}

} // namespace re::detail