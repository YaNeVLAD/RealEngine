#include <Engine/EngineContext.hpp>

#include <Core/String.hpp>
#include <RenderCore/AnimatedModel.hpp>
#include <RenderCore/Animator.hpp>
#include <RenderCore/Model.hpp>
#include <RenderCore/Texture.hpp>
#include <Runtime/Components.hpp>

#include <string_view>

uint32_t ReEngine_Scene_LoadModel(const char* pathUtf8)
{
	auto& host = Host();
	if (!host.lifecycle.initialized)
	{
		return 0;
	}

	auto& scene = host.scene.scene;
	auto& modelEntities = host.assets.modelEntities;
	for (const auto entity : modelEntities)
	{
		if (scene.IsValid(entity))
		{
			scene.DestroyEntity(entity);
		}
	}
	modelEntities.clear();

	if (!pathUtf8 || pathUtf8[0] == '\0')
	{
		scene.ConfirmChanges();
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

	for (auto&& [vertices, indices, material] : meshParts)
	{
		material.metallicFactor = 0.f;
		auto entity = scene.CreateEntity()
						  .Add<re::Dirty<re::TransformComponent>>()
						  .Add<re::TransformComponent>()
						  .Add<re::detail::OpaqueTag>()
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

		entity.Add<re::NameComponent>("Model");
		modelEntities.push_back(entity.GetEntity());
	}

	scene.ConfirmChanges();
	return static_cast<std::uint32_t>(modelEntities.size());
}

int32_t ReEngine_Scene_SetSkybox(const char* pathUtf8)
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

	scene.CreateEntity().Add<re::SkyboxComponent>(texture);
	return RE_ENGINE_OK;
}