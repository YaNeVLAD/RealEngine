#include <Engine/EngineContext.hpp>

#include <exception>

uint64_t ReEngine_ViewportCreate(const uint64_t hwnd, const uint32_t width, const uint32_t height)
{
	auto& host = Host();
	if (!host.lifecycle.initialized)
	{
		return 0;
	}
	if (hwnd == 0 || width == 0 || height == 0)
	{
		EngineLog(RE_ENGINE_LOG_ERROR, "ReEngine_ViewportCreate: invalid argument");
		return 0;
	}

	ViewportData viewport;
	viewport.width = width;
	viewport.height = height;
	try
	{
		viewport.backend = std::make_unique<re::render::FilamentRenderBackend>();
		viewport.backend->Init(reinterpret_cast<void*>(hwnd), width, height);
		viewport.backend->SetClearColor(re::Color(25, 25, 28, 255));
		viewport.system = std::make_unique<re::render::RenderSystem3D>(*viewport.backend);
	}
	catch (const std::exception&)
	{
		EngineLog(RE_ENGINE_LOG_ERROR, "ReEngine_ViewportCreate: backend init failed");
		return 0;
	}

	const std::uint64_t id = host.viewports.nextViewport++;
	host.viewports.viewports.emplace(id, std::move(viewport));
	return id;
}

int32_t ReEngine_ViewportRender(const uint64_t viewport)
{
	auto& host = Host();
	if (!host.lifecycle.initialized)
	{
		return RE_ENGINE_NOT_INITIALIZED;
	}

	const auto it = host.viewports.viewports.find(viewport);
	if (it == host.viewports.viewports.end())
	{
		EngineLog(RE_ENGINE_LOG_ERROR, "ReEngine_ViewportRender: invalid viewport");
		return RE_ENGINE_INVALID_HANDLE;
	}

	it->second.backend->BeginFrame();
	it->second.system->Update(host.scene.scene, host.scene.lastDt);
	it->second.backend->RenderFrame();
	it->second.backend->EndFrame();
	return RE_ENGINE_OK;
}

int32_t ReEngine_ViewportResize(const uint64_t viewport, const uint32_t width, const uint32_t height)
{
	auto& host = Host();
	if (!host.lifecycle.initialized)
	{
		return RE_ENGINE_NOT_INITIALIZED;
	}

	const auto it = host.viewports.viewports.find(viewport);
	if (it == host.viewports.viewports.end())
	{
		return RE_ENGINE_INVALID_HANDLE;
	}
	if (width == 0 || height == 0)
	{
		return RE_ENGINE_INVALID_ARGUMENT;
	}

	it->second.width = width;
	it->second.height = height;
	it->second.backend->Resize(width, height);
	return RE_ENGINE_OK;
}

void ReEngine_ViewportDestroy(const uint64_t viewport)
{
	auto& host = Host();
	if (!host.lifecycle.initialized)
	{
		return;
	}

	if (host.viewports.viewports.erase(viewport) == 0)
	{
		EngineLog(RE_ENGINE_LOG_WARNING, "ReEngine_ViewportDestroy: invalid viewport");
	}
}

int32_t ReEngine_ViewportSetClearColor(const uint64_t viewport, const uint32_t rgba8888)
{
	auto& host = Host();
	if (!host.lifecycle.initialized)
	{
		return RE_ENGINE_NOT_INITIALIZED;
	}

	const auto it = host.viewports.viewports.find(viewport);
	if (it == host.viewports.viewports.end())
	{
		return RE_ENGINE_INVALID_HANDLE;
	}

	it->second.backend->SetClearColor(re::Color{ rgba8888 });
	return RE_ENGINE_OK;
}