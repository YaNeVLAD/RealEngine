#pragma once

#include <Core/Math/Vector3.hpp>
#include <RenderCore/AnimatedModel.hpp>
#include <RenderCore/Assets/AssetManager.hpp>
#include <RenderCore/Model.hpp>
#include <Runtime/Application.hpp>
#include <Runtime/Components.hpp>
#include <Runtime/Internal/PrimitiveBuilder.hpp>

#include "Lab3/Asteroids/AsteroidsLayout.hpp"
#include "Lab4/Arcanoid/ArcanoidLayout.hpp"
#include "Lab4/Maze/MazeLayout.hpp"
#include "Lab4/Piano/PianoLayout.hpp"

#include <imgui.h>

#include "CameraControlSystem.hpp"

#include <deque>

struct MenuLayout final : re::Layout
{
	MenuLayout(re::Application& app, re::render::IWindow& window)
		: Layout(app)
		, m_window(window)
	{
	}

	void OnCreate() override
	{
		auto& scene = GetScene();

		scene
			.AddSystem<CameraControlSystem>()
			.WithRead<re::CameraComponent, re::TransformComponent>()
			.WithWrite<re::CameraComponent>()
			.RunOnMainThread();

		scene.CreateEntity()
			.Add<re::Dirty<re::TransformComponent>>()
			.Add<re::LightComponent>(re::LightComponent::CreatePoint(re::Color::White))
			.Add<re::TransformComponent>({
				.position = { 4.0f, 1.0f, 6.0f },
				.rotation = { 37.f, 3.f, 107.f },
			});

		if (auto camera = scene.FindFirstWith<re::CameraComponent>(); camera.IsValid())
		{
			camera.Get<re::CameraComponent>().farClip = 1000.f;
		}

		ReplaceModel("model/Model.obj");
	}

	void OnAttach() override
	{
		GetApplication().SetUIOverlayActive(false);
		m_window.SetBackgroundColor(re::Color::Gray);
	}

	void OnUIDraw() override
	{
		ImGui::Begin("Scene Settings");

		if (ImGui::CollapsingHeader("Model Loader", ImGuiTreeNodeFlags_DefaultOpen))
		{
			static char pathBuffer[256] = "model/Model.obj";
			ImGui::InputText("File Path", pathBuffer, sizeof(pathBuffer));

			if (ImGui::Button("Load Model", ImVec2(-1, 0)))
			{
				ReplaceModel(pathBuffer);
			}
		}

		if (!m_modelEntities.empty())
		{
			if (ImGui::CollapsingHeader("Model Transform", ImGuiTreeNodeFlags_DefaultOpen))
			{
				bool changed = false;

				changed |= ImGui::DragFloat3("Position", &m_modelPos.x, 0.1f);
				changed |= ImGui::DragFloat3("Rotation", &m_modelRot.x, 1.0f);
				changed |= ImGui::DragFloat3("Scale", &m_modelScale.x, 0.05f);

				if (changed)
				{
					UpdateModelTransforms();
				}
			}
		}

		ImGui::Separator();

		for (auto&& [entity, light, transform] : *GetScene().CreateView<re::LightComponent, re::TransformComponent>())
		{
			if (ImGui::CollapsingHeader("Light Editor"))
			{
				ImGui::DragFloat3("Light Pos", &transform.position.x, 0.1f);

				float color[3] = { light.diffuse.r / 255.f, light.diffuse.g / 255.f, light.diffuse.b / 255.f };
				if (ImGui::ColorEdit3("Color", color))
				{
					light.diffuse = re::Color(color[0] * 255.f, color[1] * 255.f, color[2] * 255.f);
				}

				ImGui::SliderFloat("Linear", &light.linear, 0.0f, 0.5f);
				ImGui::SliderFloat("Quad", &light.quadratic, 0.0f, 0.1f);
			}
		}

		ImGui::End();
	}

	void OnEvent(re::Event const& event) override
	{
		if (const auto* keyEvent = event.GetIf<re::Event::KeyPressed>())
		{
			if (keyEvent->key == re::Keyboard::Key::Grave && !keyEvent->alt && !keyEvent->ctrl && !keyEvent->shift)
			{
				GetApplication().SetUIOverlayActive(!GetApplication().IsUIOverlayActive());
			}
		}

		if (GetApplication().IsUIOverlayActive())
		{
			m_firstMouse = true;
			return;
		}

		if (const auto* mouseMoved = event.GetIf<re::Event::MouseMoved>())
		{
			const auto currentX = static_cast<float>(mouseMoved->position.x);
			const auto currentY = static_cast<float>(mouseMoved->position.y);

			if (m_firstMouse)
			{
				m_lastMouseX = currentX;
				m_lastMouseY = currentY;
				m_firstMouse = false;
			}

			const float xOffset = currentX - m_lastMouseX;
			const float yOffset = m_lastMouseY - currentY;

			m_lastMouseX = currentX;
			m_lastMouseY = currentY;

			for (auto&& [entity, transform, camera] : *GetScene().CreateView<re::TransformComponent, re::CameraComponent>())
			{
				if (camera.isPrimal)
				{
					camera.mouseDelta.x += xOffset;
					camera.mouseDelta.y += yOffset;
					break;
				}
			}
		}
	}

private:
	void UpdateModelTransforms()
	{
		auto& scene = GetScene();
		for (const auto entity : m_modelEntities)
		{
			if (scene.IsValid(entity))
			{
				auto& transform = scene.GetComponent<re::TransformComponent>(entity);
				transform.position = m_modelPos;
				transform.rotation = m_modelRot;
				transform.scale = m_modelScale;

				scene.MakeDirty<re::TransformComponent>(entity);
			}
		}
	}

	void ReplaceModel(const re::String& path)
	{
		auto& scene = GetScene();

		for (const auto entity : m_modelEntities)
		{
			if (scene.IsValid(entity))
			{
				scene.DestroyEntity(entity);
			}
		}
		m_modelEntities.clear();

		std::vector<re::render::MeshPart> meshParts;
		if (path.Find(".obj") != re::String::NPos)
		{
			const auto model = m_manager.Get<re::Model>(path);
			if (model)
			{
				meshParts = model->GetParts();
			}
		}
		else if (path.Find(".glb") != re::String::NPos || path.Find(".gltf") != re::String::NPos)
		{
			const auto model = m_manager.Get<re::AnimatedModel>(path);
			if (model)
			{
				meshParts = model->GetParts();
			}
		}

		for (const auto& [vertices, indices, material] : meshParts)
		{
			auto entity = scene.CreateEntity()
							  .Add<re::Dirty<re::TransformComponent>>()
							  .Add<re::TransformComponent>({
								  .position = m_modelPos,
								  .rotation = m_modelRot,
								  .scale = m_modelScale,
							  })
							  .Add<re::detail::OpaqueTag>()
							  .Add<re::MaterialComponent>(material)
							  .Add<re::StaticMeshComponent3D>(vertices, indices);

			m_modelEntities.push_back(entity.GetEntity());
		}
	}

	std::vector<re::ecs::Entity> m_modelEntities;
	re::Vector3f m_modelPos = { 0.f, 0.f, 0.f };
	re::Vector3f m_modelRot = { 0.f, 0.f, 0.f };
	re::Vector3f m_modelScale = { 1.f, 1.f, 1.f };

	re::ecs::Entity m_solid = re::ecs::Entity::INVALID_ID;
	re::ecs::Entity m_wireframe = re::ecs::Entity::INVALID_ID;
	re::AssetManager m_manager;

	re::render::IWindow& m_window;

	bool m_firstMouse = true;
	float m_lastMouseX = 0.0f;
	float m_lastMouseY = 0.0f;

	std::deque<std::vector<re::Vector3f>> m_tempCollisionMeshes;
};

class LauncherApplication final : public re::Application
{
public:
	LauncherApplication()
		: Application("Launcher application")
	{
	}

	void OnStart() override
	{
		Window().SetVSyncEnabled(true);

		AddLayout<MenuLayout>(Window());
		// AddLayout<AsteroidsLayout>(Window());
		// AddLayout<MazeLayout>(Window());
		// AddLayout<PianoLayout>(Window());
		// AddLayout<ArcanoidLayout>(Window());

		SwitchLayout<MenuLayout>();
	}

	void OnUpdate(const re::core::TimeDelta deltaTime) override
	{
		static float timeAccumulator = 0.0f;
		static int frames = 0;

		timeAccumulator += deltaTime;
		frames++;

		if (timeAccumulator >= 1.0f)
		{
			const auto fps = std::format("FPS: {} | ms: {}",
				frames,
				timeAccumulator / static_cast<float>(frames) * 1000.0f);

			// Window().SetTitle(fps);

			frames = 0;
			timeAccumulator = 0.0f;
		}
	}

	void OnEvent(re::Event const& event) override
	{
		if (const auto* e = event.GetIf<re::Event::KeyPressed>())
		{
			if (e->key == re::Keyboard::Key::F1)
			{
				SwitchLayout<MenuLayout>();
			}
			if (e->key == re::Keyboard::Key::F2)
			{
				SwitchLayout<AsteroidsLayout>();
			}
			if (e->key == re::Keyboard::Key::F3)
			{
				SwitchLayout<MazeLayout>();
			}
			if (e->key == re::Keyboard::Key::F4)
			{
				SwitchLayout<PianoLayout>();
			}
			if (e->key == re::Keyboard::Key::F5)
			{
				SwitchLayout<ArcanoidLayout>();
			}
		}
	}

	void OnStop() override
	{
	}
};
