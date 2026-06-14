#pragma once

#include <Core/Math/Vector3.hpp>
#include <ECS/Scene.hpp>
#include <RVM/VirtualMachine.hpp>
#include <Runtime/Components.hpp>

namespace re::scripting
{
inline void BindEngineAPI(rvm::VirtualMachine* vm)
{
	using namespace re::rvm;
	using namespace re::ecs;

	// @RvmNative("Engine_Spawn")
	// external fun __spawnEntity(prefabName: String, x: Double, y: Double): Int;
	vm->RegisterNative("Engine_Spawn", [vm](const std::vector<Value>& args) -> Value {
		auto prefabName = std::get<String>(args[0]);
		const auto x = static_cast<float>(std::get<Double>(args[1]));
		const auto y = static_cast<float>(std::get<Double>(args[2]));

		auto* scene = static_cast<Scene*>(vm->GetUserData());
		if (!scene)
		{
			return -1;
		}

		const auto pos = Vector3f{ x, y, 0.0f };
		const auto [v, i] = detail::PrimitiveBuilder::CreateCube(Color::Gray);
		const auto mesh = std::make_shared<StaticMesh>(v, i);

		const auto entity = scene->CreateEntity()
								.Add<Dirty<TransformComponent>>()
								.Add<TransformComponent>({
									.position = pos,
									.scale = Vector3f(1.f),
								})
								.Add<detail::OpaqueTag>()
								.Add<StaticMeshComponent3D>(mesh)
								.Add<MaterialComponent>();

		return static_cast<Int>(entity.GetEntity());
	});

	// @RvmNative("Engine_Destroy")
	// external fun __destroyEntity(entityId: Int): Unit;
	vm->RegisterNative("Engine_Destroy", [vm](const std::vector<Value>& args) -> Value {
		const auto id = static_cast<uint32_t>(std::get<Int>(args[0]));
		const auto entity = Entity(id);

		if (auto* scene = static_cast<Scene*>(vm->GetUserData()); scene && scene->IsValid(entity))
		{
			scene->DestroyEntity(entity);
		}

		return Null;
	});

	// @RvmNative("Engine_GetX")
	// external fun __getX(entityId: Int): Double;
	vm->RegisterNative("Engine_GetX", [vm](const std::vector<Value>& args) -> Value {
		const auto id = static_cast<std::uint32_t>(std::get<Int>(args[0]));
		const auto entity = Entity(id);

		if (auto* scene = static_cast<Scene*>(vm->GetUserData());
			scene && scene->IsValid(entity) && scene->HasComponent<TransformComponent>(entity))
		{
			return scene->GetComponent<TransformComponent>(entity).position.x;
		}

		return 0.0;
	});

	// @RvmNative("Engine_GetY")
	// external fun __getY(entityId: Int): Double;
	vm->RegisterNative("Engine_GetY", [vm](const std::vector<Value>& args) -> Value {
		const auto id = static_cast<std::uint32_t>(std::get<Int>(args[0]));
		const auto entity = Entity(id);

		if (auto* scene = static_cast<Scene*>(vm->GetUserData());
			scene && scene->IsValid(entity) && scene->HasComponent<TransformComponent>(entity))
		{
			return scene->GetComponent<TransformComponent>(entity).position.y;
		}

		return 0.0;
	});

	// @RvmNative("Engine_SetPosition")
	// external fun __setPosition(entityId: Int, x: Double, y: Double): Unit;
	vm->RegisterNative("Engine_SetPosition", [vm](const std::vector<Value>& args) -> Value {
		const auto id = static_cast<std::uint32_t>(std::get<Int>(args[0]));
		const auto entity = Entity(id);

		const auto x = static_cast<float>(std::get<Double>(args[1]));
		const auto y = static_cast<float>(std::get<Double>(args[2]));

		if (auto* scene = static_cast<Scene*>(vm->GetUserData());
			scene && scene->IsValid(entity) && scene->HasComponent<TransformComponent>(entity))
		{
			auto& tf = scene->GetComponent<TransformComponent>(entity);
			tf.position.x = x;
			tf.position.y = y;
			scene->MakeDirty<re::TransformComponent>(entity);
		}

		return Null;
	});

	// @RvmNative("Engine_SetVelocity")
	// external fun __setVelocity(entityId: Int, velX: Double, velY: Double): Unit;
	vm->RegisterNative("Engine_SetVelocity", [vm](const std::vector<Value>& args) -> Value {
		const auto id = static_cast<std::uint32_t>(std::get<Int>(args[0]));
		const auto entity = Entity(id);

		const auto velX = static_cast<float>(std::get<Double>(args[1]));
		const auto velY = static_cast<float>(std::get<Double>(args[2]));

		if (auto* scene = static_cast<Scene*>(vm->GetUserData()); scene && scene->IsValid(entity))
		{
			if (!scene->HasComponent<RigidBodyComponent>(entity))
			{
				scene->AddComponent<RigidBodyComponent>(entity, { .linearVelocity = { velX, velY, 0.0f } });
			}
			else
			{
				scene->GetComponent<RigidBodyComponent>(entity).linearVelocity = { velX, velY, 0.0f };
			}
		}

		return Null;
	});

	// @RvmNative("Engine_GetAxis")
	// external fun __getAxis(axisName: String): Double;
	vm->RegisterNative("Engine_GetAxis", [vm](const std::vector<Value>& args) -> Value {
		auto axisName = std::get<String>(args[0]);
		const float value = 0.0f;

		// Здесь подставь вызовы реального Input System движка. Псевдокод:
		/*
		if (axisName == "Horizontal") {
			if (re::Input::IsKeyPressed(re::Key::Right)) value += 1.0f;
			if (re::Input::IsKeyPressed(re::Key::Left)) value -= 1.0f;
		}
		// ...
		*/

		return value;
	});

	// @RvmNative("Engine_IsActionPressed")
	// external fun __isActionPressed(actionName: String): Bool;
	vm->RegisterNative("Engine_IsActionPressed", [vm](const std::vector<Value>& args) -> Value {
		auto actionName = std::get<String>(args[0]);

		// Псевдокод: bool isPressed = re::Input::IsActionPressed(actionName);
		return static_cast<Int>(0); // 0 - false, 1 - true
	});
}
} // namespace re::scripting