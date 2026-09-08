#pragma once

// #include <Core/Math/Vector3.hpp>
// #include <ECS/Scene.hpp>
// #include <RVM/VirtualMachine.hpp>
// #include <Runtime/Components.hpp>

#include <iostream>

namespace re::scripting
{

namespace internal
{

inline void NativeLog_Impl(const char* message)
{
	std::cout << message << std::endl;
}

} // namespace internal

inline void BindEngineAPI(rvm::VirtualMachine* vm)
{
	// using namespace re::rvm;
	// using namespace re::ecs;

	//// @RvmNative("Engine_Spawn")
	//// external fun __spawnEntity(prefabName: String, x: Double, y: Double): Int;
	// vm->RegisterNative("Engine_Spawn", [vm](const std::vector<Value>& args) -> Value {
	//	auto prefabName = std::get<String>(args[0]);
	//	const auto x = static_cast<float>(std::get<Double>(args[1]));
	//	const auto y = static_cast<float>(std::get<Double>(args[2]));

	//	auto* scene = static_cast<Scene*>(vm->GetUserData());
	//	if (!scene)
	//	{
	//		return -1;
	//	}

	//	const auto pos = Vector3f{ x, y, 0.0f };
	//	const auto [v, i] = detail::PrimitiveBuilder::CreateCube(Color::Gray);
	//	const auto mesh = std::make_shared<StaticMesh>(v, i);

	//	const auto entity = scene->CreateEntity()
	//							.Add<Dirty<TransformComponent>>()
	//							.Add<TransformComponent>({
	//								.position = pos,
	//								.scale = Vector3f(1.f),
	//							})
	//							.Add<detail::OpaqueTag>()
	//							.Add<StaticMeshComponent3D>(mesh)
	//							.Add<MaterialComponent>()
	//							.Add<RigidBodyComponent>({
	//								.gravityFactor = 0.f,
	//							});

	//	return static_cast<Int>(entity.GetEntity());
	//});

	//// @RvmNative("Engine_Destroy")
	//// external fun __destroyEntity(entityId: Int): Unit;
	// vm->RegisterNative("Engine_Destroy", [vm](const std::vector<Value>& args) -> Value {
	//	const auto id = static_cast<uint32_t>(std::get<Int>(args[0]));
	//	const auto entity = Entity(id);

	//	if (auto* scene = static_cast<Scene*>(vm->GetUserData()); scene && scene->IsValid(entity))
	//	{
	//		scene->DestroyEntity(entity);
	//	}

	//	return Null;
	//});

	//// @RvmNative("Engine_GetX")
	//// external fun __getX(entityId: Int): Double;
	// vm->RegisterNative("Engine_GetX", [vm](const std::vector<Value>& args) -> Value {
	//	const auto id = static_cast<std::uint32_t>(std::get<Int>(args[0]));
	//	const auto entity = Entity(id);

	//	if (auto* scene = static_cast<Scene*>(vm->GetUserData());
	//		scene && scene->IsValid(entity) && scene->HasComponent<TransformComponent>(entity))
	//	{
	//		return scene->GetComponent<TransformComponent>(entity).position.x;
	//	}

	//	return 0.0;
	//});

	//// @RvmNative("Engine_GetY")
	//// external fun __getY(entityId: Int): Double;
	// vm->RegisterNative("Engine_GetY", [vm](const std::vector<Value>& args) -> Value {
	//	const auto id = static_cast<std::uint32_t>(std::get<Int>(args[0]));
	//	const auto entity = Entity(id);

	//	if (auto* scene = static_cast<Scene*>(vm->GetUserData());
	//		scene && scene->IsValid(entity) && scene->HasComponent<TransformComponent>(entity))
	//	{
	//		return scene->GetComponent<TransformComponent>(entity).position.y;
	//	}

	//	return 0.0;
	//});

	//// @RvmNative("Engine_SetPosition")
	//// external fun __setPosition(entityId: Int, x: Double, y: Double): Unit;
	// vm->RegisterNative("Engine_SetPosition", [vm](const std::vector<Value>& args) -> Value {
	//	const auto id = static_cast<std::uint32_t>(std::get<Int>(args[0]));
	//	const auto entity = Entity(id);

	//	const auto x = static_cast<float>(std::get<Double>(args[1]));
	//	const auto y = static_cast<float>(std::get<Double>(args[2]));

	//	if (auto* scene = static_cast<Scene*>(vm->GetUserData());
	//		scene && scene->IsValid(entity) && scene->HasComponent<TransformComponent>(entity))
	//	{
	//		auto& tf = scene->GetComponent<TransformComponent>(entity);
	//		tf.position.x = x;
	//		tf.position.y = y;
	//		scene->MakeDirty<TransformComponent>(entity);
	//	}

	//	return Null;
	//});

	//// @RvmNative("Engine_SetVelocity")
	//// external fun __setVelocity(entityId: Int, velX: Double, velY: Double): Unit;
	// vm->RegisterNative("Engine_SetVelocity", [vm](const std::vector<Value>& args) -> Value {
	//	const auto id = static_cast<std::uint32_t>(std::get<Int>(args[0]));
	//	const auto entity = Entity(id);

	//	const auto velX = static_cast<float>(std::get<Double>(args[1]));
	//	const auto velY = static_cast<float>(std::get<Double>(args[2]));

	//	if (auto* scene = static_cast<Scene*>(vm->GetUserData()); scene && scene->IsValid(entity))
	//	{
	//		const auto vel = Vector3f(velX, velY, 0.0f);

	//		if (!scene->HasComponent<RigidBodyComponent>(entity))
	//		{
	//			scene->AddComponent<RigidBodyComponent>(entity,
	//				{
	//					.linearVelocity = vel,
	//					.isVelocityDirty = true,
	//				});
	//		}
	//		else
	//		{
	//			auto& rb = scene->GetComponent<RigidBodyComponent>(entity);
	//			rb.linearVelocity = vel;
	//			rb.isVelocityDirty = true;
	//		}
	//	}

	//	return Null;
	//});

	//// @RvmNative("Engine_GetAxis")
	//// external fun __getAxis(axisName: String): Double;
	// vm->RegisterNative("Engine_GetAxis", [vm](const std::vector<Value>& args) -> Value {
	//	float value = 0.0f;
	//	if (const auto axisName = std::get<String>(args[0]);
	//		axisName == "Horizontal")
	//	{
	//		if (Keyboard::IsKeyPressed(Keyboard::Key::Right)
	//			|| Keyboard::IsKeyPressed(Keyboard::Key::D))
	//		{
	//			value += 1.0f;
	//		}

	//		if (Keyboard::IsKeyPressed(Keyboard::Key::Left)
	//			|| re::Keyboard::IsKeyPressed(Keyboard::Key::A))
	//		{
	//			value -= 1.0f;
	//		}
	//	}
	//	else if (axisName == "Vertical")
	//	{
	//		if (re::Keyboard::IsKeyPressed(Keyboard::Key::Up)
	//			|| re::Keyboard::IsKeyPressed(Keyboard::Key::W))
	//		{
	//			value += 1.0f;
	//		}

	//		if (re::Keyboard::IsKeyPressed(Keyboard::Key::Down)
	//			|| re::Keyboard::IsKeyPressed(Keyboard::Key::S))
	//		{
	//			value -= 1.0f;
	//		}
	//	}

	//	return static_cast<Double>(value);
	//});

	//// @RvmNative("Engine_IsActionPressed")
	//// external fun __isActionPressed(actionName: String): Bool;
	// vm->RegisterNative("Engine_IsActionPressed", [vm](const std::vector<Value>& args) -> Value {
	//	bool isPressed = false;
	//	if (const auto actionName = std::get<String>(args[0]);
	//		actionName == "Jump")
	//	{
	//		isPressed = re::Keyboard::IsKeyPressed(Keyboard::Key::Space);
	//	}
	//	else if (actionName == "Fire")
	//	{
	//		isPressed = re::Mouse::IsButtonPressed(Mouse::Button::Left)
	//			|| re::Keyboard::IsKeyPressed(Keyboard::Key::LControl);
	//	}

	//	return static_cast<Int>(isPressed ? 1 : 0);
	//});
}
} // namespace re::scripting
