#pragma once

#include <Core/Math/Vector3.hpp>
#include <Core/Revertible.hpp>
#include <Render3D/Renderer3D.hpp>
#include <RenderCore/AnimatedModel.hpp>
#include <RenderCore/Assets/AssetManager.hpp>
#include <RenderCore/Model.hpp>
#include <Runtime/Application.hpp>
#include <Runtime/Components.hpp>
#include <Runtime/Internal/PrimitiveBuilder.hpp>

#include <Runtime/Internal/ScriptBinder.hpp>
#include <Runtime/System/ScriptSystem.hpp>
#include <Scripting/CSharp/DotNetScriptEngine.hpp>

#include "Lab3/Asteroids/AsteroidsLayout.hpp"
#include "Lab4/Arcanoid/ArcanoidLayout.hpp"
#include "Lab4/Maze/MazeLayout.hpp"
#include "Lab4/Piano/PianoLayout.hpp"

#include <imgui.h>

#include "CameraControlSystem.hpp"
#include "Lab6/BattleCityLayout.hpp"

#include <deque>
#include <iostream>
#include <string>
#include <vector>

struct EditorLayout final : re::Layout
{
	struct LightGizmoTag
	{
		bool dummy = false;
	};

	static constexpr re::Vector3f DEFAULT_LIGHT_POS = { 0.f, 2.f, 1.f };
	static constexpr re::Vector3f DEFAULT_MODEL_POS = { 0.f, 0.f, 0.f };

	EditorLayout(re::Application& app, re::render::IWindow& window)
		: Layout(app)
		, m_csharpEngine(re::runtime::ScriptBinder::CreateApiPointers())
		, m_modelPos(DEFAULT_MODEL_POS)
		, m_modelRot(re::Vector3f(0.f))
		, m_modelScale(re::Vector3f(1.f))
		, m_lightPos(DEFAULT_LIGHT_POS)
		, m_window(window)
	{
		using namespace re::file_system::literals;

		if (m_csharpEngine.LoadAssembly("RealEngineTestProj.dll"_script))
		{
			std::cout << "[EditorLayout] Game assembly loaded!\n";
		}
	}

	void OnCreate() override
	{
		auto& scene = GetScene();

		scene
			.AddSystem<CameraControlSystem>()
			.WithRead<re::CameraComponent, re::TransformComponent>()
			.WithWrite<re::CameraComponent>()
			.RunOnMainThread();

		scene
			.AddSystem<re::ScriptSystem>(&m_csharpEngine)
			.WithRead<re::ScriptComponent>()
			.WithWrite<re::ScriptComponent>()
			.RunOnMainThread();

		auto [sphereV, sphereI] = re::detail::PrimitiveBuilder::CreateSphere(re::Color::Yellow);
		auto sphereMesh = std::make_shared<re::StaticMesh>(sphereV, sphereI);

		// auto [cubeV, cubeI] = re::detail::PrimitiveBuilder::CreateCube(re::Color::Yellow);
		// auto cubeMesh = std::make_shared<re::StaticMesh>(cubeV, cubeI);

		// scene
		// 	.CreateEntity()
		// 	.Add<re::ScriptComponent>({
		// 		.Class = "Player",
		// 		.Namespace = "RealEngineTestProj",
		// 	})
		// 	.Add<re::TransformComponent>({ .position = re::Vector3f::Zero() });

		const auto lightEntity
			= scene.CreateEntity()
				  .Add<re::Dirty<re::TransformComponent>>()
				  .Add<re::LightComponent>(re::LightComponent::CreatePoint(re::Color::White))
				  .Add<re::TransformComponent>({
					  .position = DEFAULT_LIGHT_POS,
					  .rotation = { 37.f, 107.f, 3.f },
				  });

		// auto skyboxTexture = m_manager.Get<re::Texture>("model/citrus_orchard_road_puresky_4k.hdr");
		auto skyboxTexture = m_manager.Get<re::Texture>("model/grasslands_sunset_4k.hdr");
		scene.CreateEntity()
			.Add<re::SkyboxComponent>(skyboxTexture);

		// scene.CreateEntity()
		// 	.Add<re::Dirty<re::TransformComponent>>()
		// 	.Add<re::TransformComponent>({
		// 		.position = DEFAULT_LIGHT_POS,
		// 		.scale = re::Vector3f(0.25f),
		// 	})
		// 	.Add<re::detail::OpaqueTag>()
		// 	.Add<re::StaticMeshComponent3D>(sphereMesh)
		// 	.Add<re::MaterialComponent>(re::Material{ .emissionColor = re::Color::White })
		// 	.Add<LightGizmoTag>();

		m_lightEntity = lightEntity.GetEntity();

		if (auto camera = scene.FindFirstWith<re::CameraComponent>(); camera.IsValid())
		{
			camera.Get<re::CameraComponent>().farClip = 100'000.f;
			auto& transform = camera.Get<re::TransformComponent>();
			transform.position = { 0.f, 1.5f, 3.f };
		}

		// ReplaceModel("model/Fox.glb");
	}

	void OnUpdate(const re::core::TimeDelta dt) override
	{
	}

	void OnAttach() override
	{
		GetApplication().SetUIOverlayActive(false);
		m_window.SetBackgroundColor(re::Color(63, 63, 63));
	}

	void OnUIDraw() override
	{
		ImGui::Begin("Scene Settings");

		if (ImGui::CollapsingHeader("Model Loader", ImGuiTreeNodeFlags_DefaultOpen))
		{
			static char modelPathBuffer[256] = "model/Fox.glb";
			ImGui::InputText("File Path##Model", modelPathBuffer, sizeof(modelPathBuffer));

			if (ImGui::Button("Load Model", ImVec2(-1, 0)))
			{
				ReplaceModel(modelPathBuffer);
			}
		}

		if (!m_modelEntities.empty())
		{
			if (ImGui::CollapsingHeader("Model Transform", ImGuiTreeNodeFlags_DefaultOpen))
			{
				bool changed = false;

				changed |= ImGui::DragFloat3("Position", &m_modelPos.RawRef().x, 0.1f);
				changed |= ImGui::DragFloat3("Rotation", &m_modelRot.RawRef().x, 1.0f);
				changed |= ImGui::DragFloat3("Scale", &m_modelScale.RawRef().x, 0.05f);

				if (changed)
				{
					UpdateModelTransforms();
				}

				ImGui::BeginDisabled(!m_modelPos.Modified() && !m_modelRot.Modified() && !m_modelScale.Modified());
				if (ImGui::Button("Reset Transform", ImVec2(-1, 0)))
				{
					m_modelPos.Reset();
					m_modelRot.Reset();
					m_modelScale.Reset();
					UpdateModelTransforms();
				}
				ImGui::EndDisabled();
			}

			if (ImGui::CollapsingHeader("Animation", ImGuiTreeNodeFlags_DefaultOpen))
			{
				const re::AnimatedModel* animatedModel = nullptr;
				for (const auto entity : m_modelEntities)
				{
					if (GetScene().IsValid(entity) && GetScene().HasComponent<re::AnimatedMeshComponent3D>(entity))
					{
						animatedModel = GetScene().GetComponent<re::AnimatedMeshComponent3D>(entity).model.get();
						break;
					}
				}

				if (!animatedModel)
				{
					ImGui::Text("Loaded model has no animations (load a .glb).");
				}
				else
				{
					if (const auto& animations = animatedModel->Animations(); animations.empty())
					{
						ImGui::Text("Model has no animation clips.");
					}
					else
					{
						std::vector<std::string> labels;
						labels.reserve(animations.size());
						for (const auto& anim : animations)
						{
							labels.emplace_back(anim.name.empty() ? "<unnamed>" : anim.name);
						}

						std::vector<const char*> items;
						items.reserve(labels.size());
						for (const auto& label : labels)
						{
							items.push_back(label.c_str());
						}

						if (ImGui::Combo("Animation##Selector", &m_selectedAnimationIndex, items.data(), static_cast<int>(items.size())))
						{
							const int idx = m_selectedAnimationIndex;
							const auto& anim = animations[idx];
							std::cout << "[EditorLayout] Animation selected: [" << idx << "] '" << anim.name
									  << "' (duration " << anim.duration << "s)\n";

							for (const auto entity : m_modelEntities)
							{
								if (GetScene().IsValid(entity) && GetScene().HasComponent<re::AnimatedMeshComponent3D>(entity))
								{
									if (const auto& animComp = GetScene().GetComponent<re::AnimatedMeshComponent3D>(entity); animComp.animator)
									{
										animComp.animator->PlayAnimation(idx);
									}
								}
							}
						}
					}
				}
			}
		}

		ImGui::Separator();

		const auto lightView = GetScene().CreateView<re::LightComponent, re::TransformComponent>();
		const auto gizmoView = GetScene().CreateView<LightGizmoTag, re::TransformComponent>();
		for (auto&& [entity, light, transform] : *lightView)
		{
			if (ImGui::CollapsingHeader("Light Editor"))
			{
				bool changed = false;
				changed |= ImGui::DragFloat3("Light Pos", &m_lightPos.RawRef().x, 0.1f);

				if (changed)
				{
					UpdateLightTransforms(transform, gizmoView);
				}

				ImGui::BeginDisabled(!m_lightPos.Modified());
				if (ImGui::Button("Reset Light Position", ImVec2(-1, 0)))
				{
					m_lightPos.Reset();
					UpdateLightTransforms(transform, gizmoView);
				}
				ImGui::EndDisabled();

				if (auto color = light.diffuse.ToFloat(); ImGui::ColorEdit3("Color", color.Data()))
				{
					light.diffuse = re::Color::FromFloat(color);
				}

				ImGui::SliderFloat("Intensity", &light.exponent, 0.0f, 100.0f);
				ImGui::SliderFloat("Radius (Falloff)", &light.falloff, 0.0f, 500.0f);
				ImGui::SliderFloat("Ambient", &light.ambientIntensity, 0.0f, 1.0f);

				if (auto ambient = light.ambient.ToFloat(); ImGui::ColorEdit3("Ambient Color", ambient.Data()))
				{
					light.ambient = re::Color::FromFloat(ambient);
				}

				ImGui::SliderFloat("Linear", &light.linear, 0.0f, 0.5f);
				ImGui::SliderFloat("Quad", &light.quadratic, 0.0f, 0.1f);
			}
		}

		if (ImGui::CollapsingHeader("Renderer Settings", ImGuiTreeNodeFlags_DefaultOpen))
		{
			for (auto&& [camEntity, cam, camTransform] : *GetScene().CreateView<re::CameraComponent, re::TransformComponent>())
			{
				if (cam.isPrimal)
				{
					ImGui::SliderFloat("Camera ISO", &cam.iso, 25.0f, 3200.0f);
					break;
				}
			}

			if (ImGui::Button("Reload Shaders", ImVec2(-1, 0)))
			{
				re::render::Renderer3D::ReloadShaders();
			}

			static char skyboxPathBuffer[256] = "model/grasslands_sunset_4k.hdr";
			ImGui::InputText("File Path##Skybox", skyboxPathBuffer, sizeof(skyboxPathBuffer));

			if (ImGui::Button("Load Skybox", ImVec2(-1, 0)))
			{
				UpdateSkybox(skyboxPathBuffer);
			}
		}

		ImGui::End();

		ImGui::SetNextWindowPos(ImVec2(10, 10));
		ImGui::Begin("FPS Overlay", nullptr,
			ImGuiWindowFlags_NoDecoration
				| ImGuiWindowFlags_AlwaysAutoResize
				| ImGuiWindowFlags_NoSavedSettings
				| ImGuiWindowFlags_NoFocusOnAppearing
				| ImGuiWindowFlags_NoNav
				| ImGuiWindowFlags_NoMove);

		ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
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
	void UpdateSkybox(const re::String& texturePath)
	{
		const auto skyboxTexture = m_manager.Get<re::Texture>(texturePath);
		if (auto entity = GetScene().FindFirstWith<re::SkyboxComponent>(); entity.IsValid())
		{
			auto& component = entity.Get<re::SkyboxComponent>();
			component.ChangeTexture(skyboxTexture);
		}
	}

	void UpdateLightTransforms(re::TransformComponent& lightTransform, auto& gizmoView)
	{
		lightTransform.position = *m_lightPos;
		GetScene().MakeDirty<re::TransformComponent>(m_lightEntity);

		for (auto&& [gEntity, tag, gTransform] : *gizmoView)
		{
			gTransform.position = *m_lightPos;
			GetScene().MakeDirty<re::TransformComponent>(gEntity);
		}
	}

	void UpdateModelTransforms()
	{
		auto& scene = GetScene();
		for (const auto entity : m_modelEntities)
		{
			if (scene.IsValid(entity))
			{
				auto& transform = scene.GetComponent<re::TransformComponent>(entity);
				transform.position = *m_modelPos;
				transform.rotation = *m_modelRot;
				transform.scale = *m_modelScale;

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
				meshParts = model->Parts();

				const auto& skeleton = model->Skeleton();
				const auto& animations = model->Animations();
				std::cout << "[EditorLayout] AnimatedModel loaded: " << path << " | Skeleton bones: " << skeleton.size()
						  << " | Animations: " << animations.size() << "\n";
				for (std::size_t i = 0; i < animations.size(); ++i)
				{
					std::cout << "[EditorLayout]   [" << i << "] '" << animations[i].name << "' (duration " << animations[i].duration << "s)\n";
				}
			}
		}

		m_selectedAnimationIndex = 0;

		for (auto&& [vertices, indices, material] : meshParts)
		{
			material.metallicFactor = 0.f;
			auto entity = scene.CreateEntity()
							  .Add<re::Dirty<re::TransformComponent>>()
							  .Add<re::TransformComponent>({
								  .position = *m_modelPos,
								  .rotation = *m_modelRot,
								  .scale = *m_modelScale,
							  })
							  .Add<re::detail::OpaqueTag>()
							  .Add<re::MaterialComponent>(material);

			if (path.Find(".glb") != re::String::NPos)
			{
				const auto animModel = m_manager.Get<re::AnimatedModel>(path);
				const auto animator = std::make_shared<re::Animator>(animModel.get());
				animator->PlayAnimation(0);

				entity.Add<re::AnimatedMeshComponent3D>(animModel, animator);
			}
			else
			{
				entity.Add<re::StaticMeshComponent3D>(vertices, indices);
			}

			m_modelEntities.push_back(entity.GetEntity());
		}
	}

private:
	re::DotNetScriptEngine m_csharpEngine;

	std::vector<re::ecs::Entity> m_modelEntities;
	int m_selectedAnimationIndex = 0;
	re::Revertible<re::Vector3f> m_modelPos;
	re::Revertible<re::Vector3f> m_modelRot;
	re::Revertible<re::Vector3f> m_modelScale;

	re::Revertible<re::Vector3f> m_lightPos;

	re::ecs::Entity m_lightEntity = re::ecs::Entity::INVALID_ID;
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

		AddLayout<EditorLayout>(Window());
		// AddLayout<AsteroidsLayout>(Window());
		// AddLayout<MazeLayout>(Window());
		// AddLayout<PianoLayout>(Window());
		// AddLayout<ArcanoidLayout>(Window());
		// AddLayout<battle_city::BattleCityLayout>(Window());

		SwitchLayout<EditorLayout>();
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
				SwitchLayout<EditorLayout>();
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
