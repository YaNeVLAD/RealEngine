#include <Runtime/Internal/RenderSystem2D.hpp>

#include <ECS/Scene.hpp>
#include <Render2D/Renderer2D.hpp>
#include <Runtime/Components.hpp>

#include <algorithm>
#include <cmath>

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
		RenderCommand cmd;
		cmd.z = transform.position.z;
		cmd.type = RenderCmdType::Quad;
		cmd.position = transform.position;
		cmd.size = { rect.size.x * transform.scale.x, rect.size.y * transform.scale.y };
		cmd.rotation = transform.rotation.z;
		cmd.color = rect.color;

		m_renderQueue.push_back(std::move(cmd));
	}

	for (auto&& [entity, transform, circle] : *scene.CreateView<TransformComponent, CircleComponent>())
	{
		RenderCommand cmd;
		cmd.z = transform.position.z;
		cmd.type = RenderCmdType::Circle;
		cmd.position = transform.position;
		cmd.size.x = circle.radius * transform.scale.x;
		cmd.color = circle.color;

		m_renderQueue.push_back(std::move(cmd));
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
			RenderCommand cmd;
			cmd.z = transform.position.z;
			cmd.type = RenderCmdType::TexturedQuad;
			cmd.position = transform.position;
			cmd.size = {
				static_cast<float>(comp.texture->Width()) * transform.scale.x,
				static_cast<float>(comp.texture->Height()) * transform.scale.y
			};
			cmd.texture = comp.texture.get();
			cmd.color = Color::White;

			m_renderQueue.push_back(std::move(cmd));
		}
	}

	for (auto&& [entity, transform, text] : *scene.CreateView<TransformComponent, TextComponent>())
	{
		if (text.font)
		{
			RenderCommand cmd;
			cmd.z = transform.position.z;
			cmd.type = RenderCmdType::Text;
			cmd.position = transform.position;
			cmd.size.x = text.size * transform.scale.x;
			cmd.color = text.color;
			cmd.font = text.font.get();
			cmd.text = text.text;

			m_renderQueue.push_back(std::move(cmd));
		}
	}

	for (auto&& [entity, transform, mesh] : *scene.CreateView<TransformComponent, MeshComponent>())
	{
		std::vector<Vertex> transformedVertices = mesh.vertices;

		const float rad = transform.rotation.z * (3.14159265f / 180.0f);
		const float cosA = std::cos(rad);
		const float sinA = std::sin(rad);

		for (auto& v : transformedVertices)
		{
			const float scaledX = v.position.x * transform.scale.x;
			const float scaledY = v.position.y * transform.scale.y;
			const float scaledZ = v.position.z * transform.scale.z;

			const float rotatedX = scaledX * cosA - scaledY * sinA;
			const float rotatedY = scaledX * sinA + scaledY * cosA;

			v.position.x = transform.position.x + rotatedX;
			v.position.y = transform.position.y + rotatedY;

			v.position.z = transform.position.z + scaledZ;
		}

		RenderCommand cmd;
		cmd.z = transform.position.z;
		cmd.type = RenderCmdType::Mesh;
		cmd.meshVertices = std::move(transformedVertices);
		cmd.primitiveType = mesh.type;

		m_renderQueue.push_back(std::move(cmd));
	}

	std::ranges::stable_sort(m_renderQueue,
		[](const RenderCommand& a, const RenderCommand& b) {
			return a.z < b.z;
		});

	Vector2f cameraPos = { 0, 0 };
	float cameraZoom = 1.0f;

	for (auto&& [_, transform, camera] : *scene.CreateView<TransformComponent, CameraComponent>())
	{
		if (camera.isPrimal)
		{
			cameraPos = { transform.position.x, transform.position.y };
			cameraZoom = camera.zoom;
			break;
		}
	}

	render::Renderer2D::BeginScene(cameraPos, cameraZoom);

	for (const auto& cmd : m_renderQueue)
	{
		switch (cmd.type)
		{
		case RenderCmdType::Quad:
			render::Renderer2D::DrawQuad(cmd.position, cmd.size, cmd.rotation, cmd.color);
			break;
		case RenderCmdType::Circle:
			render::Renderer2D::DrawCircle(cmd.position, cmd.size.x, cmd.color);
			break;
		case RenderCmdType::TexturedQuad:
			render::Renderer2D::DrawTexturedQuad(cmd.position, cmd.size, cmd.texture, cmd.color);
			break;
		case RenderCmdType::Text:
			render::Renderer2D::DrawText(cmd.text, *cmd.font, cmd.position, cmd.size.x, cmd.color);
			break;
		case RenderCmdType::Mesh:
			render::Renderer2D::DrawMesh(cmd.meshVertices, cmd.primitiveType);
			break;
		}
	}

	render::Renderer2D::EndScene();
}

} // namespace re::detail