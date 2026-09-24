#pragma once

#include <Engine/Engine.h>

#include <ECS/Entity/Entity.hpp>
#include <ECS/Scene.hpp>
#include <RenderCore/Assets/AssetManager.hpp>
#include <RenderCore/Filament/FilamentRenderBackend.hpp>
#include <Runtime/Internal/RenderSystem3D.hpp>
#include <Runtime/Internal/ScriptBinder.hpp>
#include <Runtime/System/HierarchySystem.hpp>
#include <Runtime/System/PhysicsSystem.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

struct LifecycleContext
{
	bool initialized = false;
	ReEngine_LogCallback logCallback = nullptr;
	void* logUserdata = nullptr;
};

struct SceneContext
{
	re::ecs::Scene scene;
	std::optional<re::PhysicsSystem> physics;
	re::HierarchySystem hierarchy;
	re::AssetManager assets;
	re::scripting::EngineApiPointers api{};
	float lastDt = 1.f / 60.f;
};

struct ComponentContext
{
	// Reflection state lives in the ScriptBinder tables; this context only
	// namespaces the component/field exports. No owned state.
};

struct ViewportData
{
	std::unique_ptr<re::render::FilamentRenderBackend> backend;
	std::unique_ptr<re::render::RenderSystem3D> system;
	std::uint32_t width = 0;
	std::uint32_t height = 0;
};

struct ViewportContext
{
	std::unordered_map<std::uint64_t, ViewportData> viewports;
	std::uint64_t nextViewport = 1;
};

struct AssetContext
{
	std::vector<re::ecs::Entity> modelEntities;
};

struct EngineHost
{
	LifecycleContext lifecycle;
	SceneContext scene;
	ComponentContext components;
	ViewportContext viewports;
	AssetContext assets;
};

EngineHost& Host();

void EngineLog(int32_t level, const char* message);

void SeedCameraAndSun(re::ecs::Scene& scene);

inline constexpr std::uint64_t kInvalidEntity = re::ecs::Entity::INVALID_ID.Id();