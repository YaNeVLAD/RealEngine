#pragma once

#include <Core/Math/Vector3.hpp>

#include <cstdint>

namespace re::physics
{

using BodyHandle = std::uint32_t;
constexpr BodyHandle INVALID_BODY_HANDLE = 0xFFFFFFFF;

enum class BodyType
{
	Static,
	Kinematic,
	Dynamic
};

enum class ColliderType
{
	Box,
	Sphere,
	Capsule
};

struct Collider
{
	ColliderType type = ColliderType::Box;
	Vector3f halfExtends = { 0.5f, 0.5f, 0.5f };

	float radius = 0.5f;
	float height = 1.f;
};

struct RigidBody
{
	BodyType type = BodyType::Dynamic;
	Vector3f position = { 0.f, 0.f, 0.f };
	Vector3f rotationEuler = { 0.f, 0.f, 0.f }; // or Quaternion
	Collider collider;

	float mass = 1.0f;
	float friction = 0.2f;
	float restitution = 0.0f;
	float gravityFactor = 1.f;
	float linearDamping = 0.05f;

	Vector3f linearVelocity = { 0.f, 0.f, 0.f };

	bool isVelocityDirty = false;
	bool lockRotation = false;
	bool isSensor = false;

	std::uint32_t entityId = 0xFFFFFFFF;
	BodyHandle handle = INVALID_BODY_HANDLE;
};

struct CollisionPair
{
	std::uint32_t entityA;
	std::uint32_t entityB;
};

} // namespace re::physics