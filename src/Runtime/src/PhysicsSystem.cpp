#include <Runtime/System/PhysicsSystem.hpp>

#include <Runtime/System/HierarchySystem.hpp>

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

glm::mat4 TRSMatrix(const Vector3f& position, const Vector3f& rotationEulerDeg, const Vector3f& scale)
{
	// Same convention as HierarchySystem: T * R(YXZ euler, degrees) * S.
	glm::mat4 result = glm::translate(glm::mat4(1.0f), glm::vec3(position.x, position.y, position.z));
	result *= glm::eulerAngleYXZ(
		glm::radians(rotationEulerDeg.y),
		glm::radians(rotationEulerDeg.x),
		glm::radians(rotationEulerDeg.z));
	result = glm::scale(result, glm::vec3(scale.x, scale.y, scale.z));
	return result;
}

// Appends one mesh part (positions only) transformed into the root body frame.
void AppendMeshPart(physics::RigidBody& rb, const std::vector<Vertex>& vertices, const std::vector<std::uint32_t>& indices, const glm::mat4& rootLocal)
{
	if (vertices.empty() || indices.empty())
	{
		return;
	}
	const auto base = static_cast<std::uint32_t>(rb.meshVertices.size());
	rb.meshVertices.reserve(base + vertices.size());
	for (const auto& vertex : vertices)
	{
		const glm::vec4 p = rootLocal * glm::vec4(vertex.position.x, vertex.position.y, vertex.position.z, 1.0f);
		rb.meshVertices.emplace_back(p.x, p.y, p.z);
	}
	rb.meshIndices.reserve(rb.meshIndices.size() + indices.size());
	for (const std::uint32_t index : indices)
	{
		rb.meshIndices.push_back(base + index);
	}
}

void AppendEntityMeshes(ecs::Scene& scene, const ecs::Entity entity, physics::RigidBody& rb, const glm::mat4& rootLocal)
{
	if (scene.HasComponent<StaticMeshComponent3D>(entity))
	{
		if (const auto& meshComp = scene.GetComponent<StaticMeshComponent3D>(entity); meshComp.mesh)
		{
			AppendMeshPart(rb, meshComp.mesh->GetVertices(), meshComp.mesh->GetIndices(), rootLocal);
		}
	}
	if (scene.HasComponent<AnimatedMeshComponent3D>(entity))
	{
		if (const auto& animComp = scene.GetComponent<AnimatedMeshComponent3D>(entity); animComp.model)
		{
			for (const auto& part : animComp.model->Parts())
			{
				AppendMeshPart(rb, part.vertices, part.indices, rootLocal);
			}
		}
	}
}

bool FillColliderMeshData(ecs::Scene& scene, const ecs::Entity entity, const TransformComponent& rootTransform, physics::RigidBody& rb)
{
	rb.meshVertices.clear();
	rb.meshIndices.clear();
	rb.collider.vertices = nullptr;
	rb.collider.vertexCount = 0;
	rb.collider.indices = nullptr;
	rb.collider.indexCount = 0;

	const glm::mat4 rootWorld = TRSMatrix(rootTransform.position, rootTransform.rotation, rootTransform.scale);
	const glm::mat4 toBody = glm::inverse(TRSMatrix(rootTransform.position, rootTransform.rotation, { 1.f, 1.f, 1.f }));
	glm::vec3 invScale(1.0f);
	if (std::abs(rootTransform.scale.x) > 1e-6f)
	{
		invScale.x = 1.0f / rootTransform.scale.x;
	}
	if (std::abs(rootTransform.scale.y) > 1e-6f)
	{
		invScale.y = 1.0f / rootTransform.scale.y;
	}
	if (std::abs(rootTransform.scale.z) > 1e-6f)
	{
		invScale.z = 1.0f / rootTransform.scale.z;
	}

	AppendEntityMeshes(scene, entity, rb, glm::mat4(1.0f));

	for (auto&& [child, hierarchy] : *scene.CreateView<HierarchyComponent>())
	{
		if (hierarchy.parent.Id() != entity.Id())
		{
			continue;
		}
		glm::mat4 local = toBody * rootWorld
			* TRSMatrix(hierarchy.localPosition, hierarchy.localRotation, hierarchy.localScale);
		local[0] *= invScale.x;
		local[1] *= invScale.y;
		local[2] *= invScale.z;
		AppendEntityMeshes(scene, child, rb, local);
	}

	if (rb.meshVertices.empty() || rb.meshIndices.empty())
	{
		rb.meshVertices.clear();
		rb.meshIndices.clear();
		return false;
	}

	rb.collider.vertices = rb.meshVertices.data();
	rb.collider.vertexCount = rb.meshVertices.size();
	rb.collider.indices = rb.meshIndices.data();
	rb.collider.indexCount = rb.meshIndices.size();
	return true;
}

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
			FillColliderMeshData(scene, entity, transform, rb);

			rb.handle = m_world->CreateBody(rb);
		}

		if (rb.isShapeDirty)
		{
			rb.position = transform.position;
			rb.rotationEuler = transform.rotation;
			rb.scale = transform.scale;
			FillColliderMeshData(scene, entity, transform, rb);
			m_world->DestroyBody(rb.handle);
			rb.handle = m_world->CreateBody(rb);
			rb.isShapeDirty = false;
		}

		if (const auto pos = m_world->GetPosition(rb.handle); HasChanged(transform.position, pos))
		{
			m_world->SetPosition(rb.handle, transform.position);
		}
		if (HasChanged(transform.scale, rb.scale))
		{
			rb.scale = transform.scale;
			m_world->UpdateScale(rb.handle, rb);
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