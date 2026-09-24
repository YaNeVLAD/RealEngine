#include <Engine/EngineContext.hpp>

#include <Physics/Core.hpp>

#include <filesystem>

int32_t ReEngine_GetVersion(void)
{
	return static_cast<int32_t>(RE_ENGINE_API_VERSION);
}

int32_t ReEngine_Initialize(const ReEngine_InitArgs* args)
{
	auto& [initialized, logCallback, logUserdata] = Host().lifecycle;
	if (initialized)
	{
		return RE_ENGINE_ALREADY_INITIALIZED;
	}
	if (!args)
	{
		return RE_ENGINE_INVALID_ARGUMENT;
	}
	if (args->version != RE_ENGINE_API_VERSION)
	{
		return RE_ENGINE_VERSION_MISMATCH;
	}
	if (args->flags != 0)
	{
		return RE_ENGINE_INVALID_ARGUMENT;
	}

	logCallback = args->logCallback;
	logUserdata = args->logUserdata;

	if (args->assetsDirUtf8 && args->assetsDirUtf8[0] != '\0'
		&& !std::filesystem::exists(std::filesystem::path(args->assetsDirUtf8)))
	{
		EngineLog(RE_ENGINE_LOG_ERROR, "ReEngine_Initialize: assets directory does not exist");
		return RE_ENGINE_INVALID_ARGUMENT;
	}

	if (!re::physics::Init())
	{
		EngineLog(RE_ENGINE_LOG_ERROR, "ReEngine_Initialize: physics init failed");
		return RE_ENGINE_FAILED;
	}

	auto& scene = Host().scene;
	re::runtime::ScriptBinder::SetActiveScene(&scene.scene);
	scene.api = re::runtime::ScriptBinder::CreateApiPointers();

	SeedCameraAndSun(scene.scene);
	initialized = true;
	EngineLog(RE_ENGINE_LOG_INFO, "ReEngine initialized");
	return RE_ENGINE_OK;
}

void ReEngine_Shutdown(void)
{
	auto& host = Host();
	if (!host.lifecycle.initialized)
	{
		return;
	}

	host.viewports.viewports.clear();
	re::physics::Shutdown();
	re::runtime::ScriptBinder::SetActiveScene(nullptr);
	host.lifecycle.initialized = false;
}

int32_t ReEngine_Update(const float deltaSeconds)
{
	auto& host = Host();
	if (!host.lifecycle.initialized)
	{
		return RE_ENGINE_NOT_INITIALIZED;
	}

	const float dt = std::clamp(deltaSeconds, 0.f, 0.1f);
	host.scene.lastDt = dt;

	host.scene.physics.Update(host.scene.scene, dt);
	host.scene.hierarchy.Update(host.scene.scene, dt);
	host.scene.scene.ConfirmChanges();
	return RE_ENGINE_OK;
}

void ReEngine_SetLogCallback(const ReEngine_LogCallback callback, void* userdata)
{
	auto& lifecycle = Host().lifecycle;
	lifecycle.logCallback = callback;
	lifecycle.logUserdata = userdata;
}