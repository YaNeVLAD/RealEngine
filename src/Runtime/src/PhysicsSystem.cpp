#include <Runtime/System/PhysicsSystem.hpp>

namespace
{

bool HasChanged(re::Vector3f const& newValue, re::Vector3f const& oldValue)
{
	const bool xPosChanged = std::abs(newValue.x - oldValue.x) > std::numeric_limits<float>::epsilon();
	const bool yPosChanged = std::abs(newValue.y - oldValue.y) > std::numeric_limits<float>::epsilon();
	const bool zPosChanged = std::abs(newValue.z - oldValue.z) > std::numeric_limits<float>::epsilon();

	return xPosChanged || yPosChanged || zPosChanged;
}

} // namespace

namespace re
{

PhysicsSystem::PhysicsSystem(ecs::Scene& scene)
{
	m_world = physics::IPhysicsWorld::Create();
	m_world->Init();

	scene.GetRegistry().on_destroy<physics::RigidBody>().connect<&PhysicsSystem::OnRigidBodyDestroyed>(this);
}

void PhysicsSystem::Update(ecs::Scene& scene, const core::TimeDelta dt)
{
	if (!m_world)
	{
		return;
	}

	const auto view = scene.CreateView<TransformComponent, physics::RigidBody>();
	for (auto&& [entity, transform, rb] : *view)
	{
		if (rb.handle == physics::INVALID_BODY_HANDLE)
		{
			rb.position = transform.position;
			rb.rotationEuler = transform.rotation;
			rb.scale = transform.scale;
			rb.entityId = static_cast<std::uint32_t>(entity.Id());

			rb.handle = m_world->CreateBody(rb);
		}

		if (scene.HasComponent<Dirty<TransformComponent>>(entity))
		{
			if (const auto pos = m_world->GetPosition(rb.handle); HasChanged(transform.position, pos))
			{
				m_world->SetPosition(rb.handle, transform.position);
			}
			if (HasChanged(transform.scale, rb.scale))
			{
				rb.scale = transform.scale;
				m_world->UpdateScale(rb.handle, rb);
			}
		}

		if (rb.isVelocityDirty)
		{
			m_world->SetLinearVelocity(rb.handle, rb.linearVelocity);
			rb.isVelocityDirty = false;
		}
	}

	m_world->Step(dt);

	for (auto&& [entity, transform, rb] : *view)
	{
		if (rb.handle != physics::INVALID_BODY_HANDLE
			&& (rb.type == physics::BodyType::Dynamic || rb.type == physics::BodyType::Kinematic))
		{
			const Vector3f newPosition = m_world->GetPosition(rb.handle);
			rb.linearVelocity = m_world->GetLinearVelocity(rb.handle);

			if (HasChanged(newPosition, transform.position))
			{
				transform.position = newPosition;

				scene.MakeDirty<TransformComponent>(entity);
			}
		}
	}

	const auto collisions = m_world->GetAndClearCollisions();
	const auto eventView = scene.CreateView<PhysicsEventsComponent>();
	if (eventView->begin() == eventView->end())
	{
		scene.CreateEntity().Add<PhysicsEventsComponent>({ collisions });
	}
	else
	{
		for (auto&& [e, comp] : *eventView)
		{
			comp.collisions = collisions;
		}
	}
}

void PhysicsSystem::OnRigidBodyDestroyed(entt::registry& registry, const entt::entity entity) const
{
	RE_ASSERT(m_world, "m_world is nullptr");

	if (auto& rb = registry.get<physics::RigidBody>(entity); rb.handle != physics::INVALID_BODY_HANDLE)
	{
		m_world->DestroyBody(rb.handle);
		rb.handle = physics::INVALID_BODY_HANDLE;
	}
}

} // namespace re