#include <Engine/EngineContext.hpp>

#include <Runtime/System/HierarchySystem.hpp>

#include <algorithm>
#include <vector>

namespace
{

void CollectChildren(re::ecs::Scene& scene, const re::ecs::Entity entity, std::vector<re::ecs::Entity>& out)
{
	for (auto&& [child, hierarchy] : *scene.CreateView<re::HierarchyComponent>())
	{
		if (hierarchy.parent == entity)
		{
			out.push_back(child);
		}
	}
}

} // namespace

uint64_t RE_CALL ReEngine_Scene_CreateEntity()
{
	const auto& host = Host();
	if (!host.lifecycle.initialized)
	{
		return kInvalidEntity;
	}

	return host.scene.api.Scene_CreateEntity();
}

int32_t RE_CALL ReEngine_Scene_IsEntityValid(const uint64_t entity)
{
	const auto& host = Host();
	if (!host.lifecycle.initialized)
	{
		return 0;
	}

	return host.scene.api.Scene_IsEntityValid(entity) ? 1 : 0;
}

void RE_CALL ReEngine_Scene_DestroyEntity(const uint64_t entity)
{
	const auto& host = Host();
	if (!host.lifecycle.initialized)
	{
		return;
	}

	host.scene.api.Scene_DestroyEntity(entity);
}

uint64_t RE_CALL ReEngine_Entity_GetParent(const uint64_t entity)
{
	auto& host = Host();
	if (!host.lifecycle.initialized)
	{
		return kInvalidEntity;
	}

	auto& scene = host.scene.scene;
	const auto target = re::ecs::Entity{ entity };
	if (!scene.IsValid(target) || !scene.HasComponent<re::HierarchyComponent>(target))
	{
		return kInvalidEntity;
	}

	return scene.GetComponent<re::HierarchyComponent>(target).parent.Id();
}

int32_t RE_CALL ReEngine_Entity_GetChildren(const uint64_t entity, uint64_t* outIds, const uint32_t capacity, uint32_t* outTotal)
{
	auto& host = Host();
	if (!host.lifecycle.initialized)
	{
		return RE_ENGINE_NOT_INITIALIZED;
	}
	if (!outIds || !outTotal)
	{
		return RE_ENGINE_INVALID_ARGUMENT;
	}

	auto& scene = host.scene.scene;
	const auto target = re::ecs::Entity{ entity };
	if (!scene.IsValid(target))
	{
		return RE_ENGINE_INVALID_HANDLE;
	}

	std::vector<re::ecs::Entity> children;
	CollectChildren(scene, target, children);

	const auto total = static_cast<std::uint32_t>(children.size());
	const auto count = std::min(total, capacity);
	for (std::uint32_t i = 0; i < count; ++i)
	{
		outIds[i] = children[i].Id();
	}
	*outTotal = total;
	return RE_ENGINE_OK;
}

uint32_t RE_CALL ReEngine_Scene_GetEntityCount()
{
	const auto& host = Host();
	if (!host.lifecycle.initialized)
	{
		return 0;
	}

	return host.scene.api.Scene_GetEntityCount();
}

int32_t RE_CALL ReEngine_Scene_GetEntities(uint64_t* outIds, const uint32_t capacity, uint32_t* outTotal)
{
	const auto& host = Host();
	if (!host.lifecycle.initialized)
	{
		return RE_ENGINE_NOT_INITIALIZED;
	}
	if (!outIds || !outTotal)
	{
		return RE_ENGINE_INVALID_ARGUMENT;
	}

	return host.scene.api.Scene_GetEntities(outIds, capacity, outTotal) ? RE_ENGINE_OK : RE_ENGINE_FAILED;
}

uint32_t RE_CALL ReEngine_Scene_ClearEntities()
{
	const auto& host = Host();
	if (!host.lifecycle.initialized)
	{
		return 0;
	}

	return host.scene.api.Scene_ClearEntities();
}

uint64_t RE_CALL ReEngine_Scene_SpawnPrimitive(const int32_t kind, const uint32_t rgba)
{
	const auto& host = Host();
	if (!host.lifecycle.initialized)
	{
		return kInvalidEntity;
	}

	return host.scene.api.Scene_SpawnPrimitive(kind, rgba);
}

void RE_CALL ReEngine_Scene_ConfirmChanges()
{
	const auto& host = Host();
	if (!host.lifecycle.initialized)
	{
		return;
	}

	host.scene.api.Scene_ConfirmChanges();
}