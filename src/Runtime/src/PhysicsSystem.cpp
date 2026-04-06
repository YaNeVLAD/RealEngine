#include <Runtime/System/PhysicsSystem.hpp>

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
	// ---------------------------------------------------------
	// ФАЗА 1: ECS -> Physics (Создание тел и ручное перемещение)
	// ---------------------------------------------------------
	for (auto&& [entity, transform, rb] : *view)
	{
		// Если тело еще не зарегистрировано в Jolt
		if (rb.handle == physics::INVALID_BODY_HANDLE)
		{
			// Копируем параметры из Transform в дескриптор тела
			rb.position = transform.position;
			rb.rotationEuler = transform.rotation;

			// Создаем тело в физическом движке и сохраняем его ID
			rb.entityId = static_cast<std::uint32_t>(entity.Id());
			rb.handle = m_world->CreateBody(rb);
		}
		// Если мы телепортировали объект кодом (добавили DirtyTag)
		else
		{
			if (scene.HasComponent<Dirty<TransformComponent>>(entity))
			{
				m_world->SetPosition(rb.handle, transform.position);
			}

			if (rb.isVelocityDirty)
			{
				m_world->SetLinearVelocity(rb.handle, rb.linearVelocity);
				rb.isVelocityDirty = false;
			}
		}
	}

	// ---------------------------------------------------------
	// ФАЗА 2: Симуляция
	// ---------------------------------------------------------
	// Делаем шаг физики. Jolt сам просчитает гравитацию и столкновения
	m_world->Step(dt);

	// ---------------------------------------------------------
	// ФАЗА 3: Physics -> ECS (Обновление позиций для рендера)
	// ---------------------------------------------------------
	for (auto&& [entity, transform, rb] : *view)
	{
		if (rb.handle != physics::INVALID_BODY_HANDLE
			&& (rb.type == physics::BodyType::Dynamic || rb.type == physics::BodyType::Kinematic))
		{
			const Vector3f newPosition = m_world->GetPosition(rb.handle);
			rb.linearVelocity = m_world->GetLinearVelocity(rb.handle);

			const bool xPosChanged = std::abs(transform.position.x - newPosition.x) > std::numeric_limits<float>::epsilon();
			const bool yPosChanged = std::abs(transform.position.y - newPosition.y) > std::numeric_limits<float>::epsilon();
			const bool zPosChanged = std::abs(transform.position.z - newPosition.z) > std::numeric_limits<float>::epsilon();

			const bool isMoved = xPosChanged || yPosChanged || zPosChanged;
			if (isMoved)
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