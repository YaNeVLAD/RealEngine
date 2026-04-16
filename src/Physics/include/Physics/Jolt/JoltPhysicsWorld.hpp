#pragma once

#include <Physics/Export.hpp>

#include <Physics/IPhysicsWorld.hpp>

namespace re::physics
{

class RE_PHYSICS_API JoltPhysicsWorld final : public IPhysicsWorld
{
public:
	void Init() override;

	void Step(float deltaTime) override;

	BodyHandle CreateBody(const RigidBody& desc) override;
	void DestroyBody(BodyHandle handle) override;

	void SetPosition(BodyHandle handle, const Vector3f& position) override;
	[[nodiscard]] Vector3f GetPosition(BodyHandle handle) const override;

	void SetLinearVelocity(BodyHandle handle, const Vector3f& velocity) override;
	[[nodiscard]] Vector3f GetLinearVelocity(BodyHandle handle) const override;

	void UpdateScale(BodyHandle handle, const RigidBody& desc) override;

	std::vector<CollisionPair> GetAndClearCollisions() override;

private:
	struct JoltState;
	std::unique_ptr<JoltState> m_state;
};

} // namespace re::physics