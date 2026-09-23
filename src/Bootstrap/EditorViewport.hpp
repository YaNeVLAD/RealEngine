#pragma once

#include <ECS/Scene.hpp>
#include <Runtime/Components.hpp>
#include <Runtime/Internal/RenderSystem3D.hpp>
#include <Runtime/Internal/ScriptBinder.hpp>

#if defined(RE_USE_FILAMENT_RENDER)
#include <RenderCore/Filament/FilamentRenderBackend.hpp>
#endif

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <memory>

class EditorViewport
{
public:
	bool Create(const std::uint64_t hwnd, const std::uint32_t width, const std::uint32_t height)
	{
		if (m_backend || hwnd == 0 || width == 0 || height == 0)
		{
			return false;
		}

		m_backend = std::make_unique<re::render::FilamentRenderBackend>();
		m_system = std::make_unique<re::render::RenderSystem3D>(*m_backend);
		m_backend->Init(reinterpret_cast<void*>(hwnd), width, height);
		m_backend->SetClearColor(re::Color(25, 25, 28, 255));

		re::runtime::ScriptBinder::SetActiveScene(&m_scene);
		return true;
	}

	bool Render()
	{
		if (!m_backend || !m_system)
		{
			return false;
		}

		using Clock = std::chrono::steady_clock;
		const auto now = Clock::now();
		float dt = 1.0f / 60.0f;
		if (m_lastTime != Clock::time_point{})
		{
			dt = std::min(std::chrono::duration<float>(now - m_lastTime).count(), 0.1f);
		}
		m_lastTime = now;

		m_backend->BeginFrame();
		m_system->Update(m_scene, dt);
		m_backend->RenderFrame();
		m_backend->EndFrame();
		return true;
	}

	void Resize(const std::uint32_t width, const std::uint32_t height) const
	{
		if (m_backend && width > 0 && height > 0)
		{
			m_backend->Resize(width, height);
		}
	}

	void Destroy()
	{
		m_system.reset();
		if (m_backend)
		{
			m_backend->Shutdown();
			m_backend.reset();
		}
		re::runtime::ScriptBinder::SetActiveScene(nullptr);
	}

	re::ecs::Scene& Scene()
	{
		return m_scene;
	}

private:
	re::ecs::Scene m_scene;
	std::unique_ptr<re::render::FilamentRenderBackend> m_backend;
	std::unique_ptr<re::render::RenderSystem3D> m_system;

	std::chrono::steady_clock::time_point m_lastTime{};
};
