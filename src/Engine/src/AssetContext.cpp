#include <Engine/EngineContext.hpp>

#include <Core/String.hpp>
#include <RenderCore/AnimatedModel.hpp>
#include <RenderCore/Animator.hpp>
#include <RenderCore/Model.hpp>
#include <RenderCore/Texture.hpp>
#include <Runtime/Components.hpp>
#include <Runtime/System/HierarchySystem.hpp>

#include <filesystem>
#include <string>
#include <string_view>

uint32_t RE_CALL ReEngine_Scene_LoadModel(const char* pathUtf8)
{
	auto& host = Host();
	if (!host.lifecycle.initialized)
	{
		return 0;
	}

	auto& scene = host.scene.scene;
	auto& modelEntities = host.assets.modelEntities;

	if (!pathUtf8 || pathUtf8[0] == '\0')
	{
		return 0;
	}

	const re::String path(std::string_view{ pathUtf8 });

	std::vector<re::render::MeshPart> meshParts;
	if (path.Find(".obj") != re::String::NPos)
	{
		if (const auto model = host.scene.assets.Get<re::Model>(path))
		{
			meshParts = model->GetParts();
		}
	}
	else if (path.Find(".glb") != re::String::NPos || path.Find(".gltf") != re::String::NPos)
	{
		if (const auto model = host.scene.assets.Get<re::AnimatedModel>(path))
		{
			meshParts = model->Parts();
		}
	}

	if (meshParts.empty())
	{
		return 0;
	}

	const std::string baseName = std::filesystem::path(path.ToString()).stem().string();
	const auto parent = scene.CreateEntity()
							.Add<re::Dirty<re::TransformComponent>>()
							.Add<re::TransformComponent>()
							.Add<re::NameComponent>(re::String(baseName));
	const re::ecs::Entity parentEntity = parent.GetEntity();
	modelEntities.push_back(parentEntity);

	std::uint32_t partIndex = 0;
	for (auto&& [vertices, indices, material] : meshParts)
	{
		material.metallicFactor = 0.f;
		auto entity = scene.CreateEntity()
						  .Add<re::Dirty<re::TransformComponent>>()
						  .Add<re::TransformComponent>()
						  .Add<re::detail::OpaqueTag>()
						  .Add<re::HierarchyComponent>(parentEntity)
						  .Add<re::MaterialComponent>(material);

		if (path.Find(".glb") != re::String::NPos || path.Find(".gltf") != re::String::NPos)
		{
			const auto animModel = host.scene.assets.Get<re::AnimatedModel>(path);
			const auto animator = std::make_shared<re::Animator>(animModel.get());
			animator->PlayAnimation(0);

			entity.Add<re::AnimatedMeshComponent3D>(animModel, animator);
		}
		else
		{
			entity.Add<re::StaticMeshComponent3D>(vertices, indices);
		}

		entity.Add<re::NameComponent>(re::String(baseName + "_Part_" + std::to_string(partIndex)));
		++partIndex;
	}

	return partIndex;
}

int32_t RE_CALL ReEngine_Scene_SetSkybox(const char* pathUtf8)
{
	auto& host = Host();
	if (!host.lifecycle.initialized)
	{
		return RE_ENGINE_NOT_INITIALIZED;
	}
	if (!pathUtf8 || pathUtf8[0] == '\0')
	{
		return RE_ENGINE_INVALID_ARGUMENT;
	}

	const auto texture = host.scene.assets.Get<re::Texture>(re::String(std::string_view{ pathUtf8 }));
	if (!texture)
	{
		EngineLog(RE_ENGINE_LOG_WARNING, "ReEngine_SceneSetSkybox: texture not found");
		return RE_ENGINE_FAILED;
	}

	auto& scene = host.scene.scene;
	if (auto entity = scene.FindFirstWith<re::SkyboxComponent>(); entity.IsValid())
	{
		entity.Get<re::SkyboxComponent>().ChangeTexture(texture);
		return RE_ENGINE_OK;
	}

	scene.CreateEntity()
		.Add<re::SkyboxComponent>(texture)
		.Add<re::NameComponent>("Skybox");

	return RE_ENGINE_OK;
}