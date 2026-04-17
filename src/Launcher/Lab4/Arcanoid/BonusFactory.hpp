#pragma once

#include <ECS/Scene.hpp>
#include <RenderCore/StaticMesh.hpp>
#include <Runtime/Assets/AssetManager.hpp>

#include "Components.hpp"
#include "Constants.hpp"
#include "Runtime/Internal/PrimitiveBuilder.hpp"

#include <memory>
#include <string>

namespace arcanoid
{

class BonusFactory
{
public:
	BonusFactory() = default;

	void Spawn(re::ecs::Scene& scene, const re::Vector3f& pos)
	{
		static auto [v, i] = re::detail::PrimitiveBuilder::CreateCube(re::Color::White);
		static auto mesh = std::make_shared<re::StaticMesh>(v, i);

		const auto type = static_cast<BonusType>(std::rand() % 6 + 1);

		const re::String texturePath = "arcanoid/bonus_" + std::to_string(static_cast<int>(type)) + ".png";
		const auto texture = m_assetManager.Get<re::Texture>(texturePath);

		scene.CreateEntity()
			.Add<BonusComponent>(type)
			.Add<re::RigidBodyComponent>({
				.type = re::physics::BodyType::Kinematic,
				.collider = { re::physics::ColliderType::Box },
				.friction = 0.f,
				.gravityFactor = 0.f,
				.linearDamping = 0.f,
				.linearVelocity = { 0.f, 0.f, constants::BonusDropSpeed },
				.isVelocityDirty = true,
				.isSensor = true,
			})
			.Add<re::detail::OpaqueTag>()
			.Add<re::Dirty<re::TransformComponent>>()
			.Add<re::TransformComponent>({
				.position = pos,
				.rotation = { 0.f, 0.f, 180.f },
				.scale = { 0.75f, 0.75f, 0.75f },
			})
			.Add<re::MaterialComponent>(re::Material{ .texture = texture })
			.Add<re::StaticMeshComponent3D>(mesh);
	}

private:
	re::AssetManager m_assetManager;
};

} // namespace arcanoid