#pragma once

#include <Physics/Export.hpp>

#include <Physics/Types.hpp>

#include <memory>
#include <vector>

namespace re::physics
{

class RE_PHYSICS_API IPhysicsWorld
{
public:
	virtual ~IPhysicsWorld() = default;

	virtual void Init() = 0;

	virtual void Step(float deltaTime) = 0;

	virtual BodyHandle CreateBody(const RigidBody& desc) = 0;
	virtual void DestroyBody(BodyHandle handle) = 0;

	virtual void SetPosition(BodyHandle handle, const Vector3f& position) = 0;
	[[nodiscard]] virtual Vector3f GetPosition(BodyHandle handle) const = 0;

	virtual void SetLinearVelocity(BodyHandle handle, const Vector3f& velocity) = 0;
	[[nodiscard]] virtual Vector3f GetLinearVelocity(BodyHandle handle) const = 0;

	virtual std::vector<CollisionPair> GetAndClearCollisions() = 0;

	static std::unique_ptr<IPhysicsWorld> Create();
};

} // namespace re::physics