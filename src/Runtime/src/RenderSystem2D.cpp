#include <Runtime/Internal/RenderSystem2D.hpp>

#include <ECS/Scene/Scene.hpp>
#include <Render2D/Renderer2D.hpp>
#include <Runtime/Components.hpp>

namespace
{

int GetZIndex(re::ecs::Scene& scene, const re::ecs::Entity entity)
{
	if (!scene.HasComponent<re::ZIndexComponent>(entity))
	{
		return 0;
	}

	return scene.GetComponent<re::ZIndexComponent>(entity).value;
}

} // namespace

namespace re::detail
{

RenderSystem2D::RenderSystem2D(render::IWindow& window)
	: m_window(window)
{
}

void RenderSystem2D::Update(ecs::Scene& scene, core::TimeDelta)
{
	m_renderQueue.clear();
	for (auto&& [entity, transform, rect] : *scene.CreateView<TransformComponent, RectangleComponent>())
	{
		Vector2f size = {
			rect.size.x * transform.scale.x,
			rect.size.y * transform.scale.y
		};

		render::Renderer2D::DrawQuad(
			transform.position,
			size,
			transform.rotation,
			rect.color);

		const int z = GetZIndex(scene, entity);
		m_renderQueue.push_back(
			{ z, [=] {
				 render::Renderer2D::DrawQuad(
					 transform.position, size, transform.rotation, rect.color);
			 } });
	}

	for (auto&& [entity, transform, circle] : *scene.CreateView<TransformComponent, CircleComponent>())
	{
		const int z = GetZIndex(scene, entity);

		m_renderQueue.push_back(
			{ z, [=] {
				 render::Renderer2D::DrawCircle(
					 transform.position,
					 circle.radius * transform.scale.x,
					 circle.color);
			 } });
	}

	for (auto&& [entity, transform, comp] : *scene.CreateView<TransformComponent, DynamicTextureComponent>())
	{
		if (comp.isDirty && !comp.image.IsEmpty())
		{
			if (!comp.texture
				|| comp.texture->Width() != comp.image.Width()
				|| comp.texture->Height() != comp.image.Height())
			{
				comp.texture = std::make_shared<Texture>(
					comp.image.Width(),
					comp.image.Height());
			}

			comp.texture->SetData(comp.image.Data(), comp.image.Size());

			comp.isDirty = false;
		}

		if (comp.texture)
		{
			const int z = GetZIndex(scene, entity);
			const auto texPtr = comp.texture;
			m_renderQueue.push_back(
				{ z, [=] {
					 const Vector2f size = {
						 (float)texPtr->Width() * transform.scale.x,
						 (float)texPtr->Height() * transform.scale.y
					 };

					 render::Renderer2D::DrawTexturedQuad(
						 transform.position,
						 size,
						 texPtr.get(),
						 Color::White);
				 } });
		}
	}

	for (auto&& [entity, transform, text] : *scene.CreateView<TransformComponent, TextComponent>())
	{
		if (text.font)
		{
			const int z = GetZIndex(scene, entity);
			m_renderQueue.push_back(
				{ z, [=] {
					 render::Renderer2D::DrawText(
						 text.text, *text.font,
						 transform.position, text.size * transform.scale.x,
						 text.color);
				 } });
		}
	}

	std::ranges::stable_sort(m_renderQueue,
		[](const RenderObject& a, const RenderObject& b) {
			return a.zIndex < b.zIndex;
		});

	// RENDER
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

	for (const auto& [_, drawCall] : m_renderQueue)
	{
		drawCall();
	}

	render::Renderer2D::EndScene();
}

} // namespace re::detail