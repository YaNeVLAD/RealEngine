#include <Engine/EngineContext.hpp>

#include <Runtime/Components.hpp>

EngineHost& Host()
{
	static EngineHost host;
	return host;
}

void EngineLog(const int32_t level, const char* message)
{
	if (const auto& lifecycle = Host().lifecycle; lifecycle.logCallback && message)
	{
		lifecycle.logCallback(level, message, lifecycle.logUserdata);
	}
}

void SeedCameraAndSun(re::ecs::Scene& scene)
{
	if (auto camera = scene.FindFirstWith<re::CameraComponent>(); camera.IsValid())
	{
		camera.Get<re::CameraComponent>().farClip = 100'000.f;
		auto& transform = camera.Get<re::TransformComponent>();
		transform.position = { 0.f, 1.5f, 3.f };
		transform.rotation = { -26.6f, 0.f, 0.f };
	}
	else
	{
		scene.CreateEntity()
			.Add<re::Dirty<re::TransformComponent>>()
			.Add<re::TransformComponent>({
				.position = { 0.f, 1.5f, 3.f },
				.rotation = { -26.6f, 0.f, 0.f },
			})
			.Add<re::CameraComponent>();
	}

	scene.CreateEntity()
		.Add<re::Dirty<re::TransformComponent>>()
		.Add<re::TransformComponent>({ .position = { 0.f, 2.f, 1.f } })
		.Add<re::LightComponent>(re::LightComponent::CreateDirectional(re::Color::White));
}