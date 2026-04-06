#include <Physics/Jolt/JoltPhysicsWorld.hpp>

#include <Core/Assert.hpp>

#include <Jolt/Jolt.h>

#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include <iostream>

namespace
{

namespace Layers
{

constexpr JPH::ObjectLayer NON_MOVING = 0;
constexpr JPH::ObjectLayer MOVING = 1;
constexpr JPH::ObjectLayer NUM_LAYERS = 2;

} // namespace Layers

namespace BroadPhaseLayers
{

constexpr JPH::BroadPhaseLayer NON_MOVING(0);
constexpr JPH::BroadPhaseLayer MOVING(1);
constexpr JPH::uint NUM_LAYERS(2);

} // namespace BroadPhaseLayers

class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
{
public:
	BPLayerInterfaceImpl()
	{
		m_objectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
		m_objectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
	}

	JPH::uint GetNumBroadPhaseLayers() const override { return BroadPhaseLayers::NUM_LAYERS; }

	JPH::BroadPhaseLayer GetBroadPhaseLayer(const JPH::ObjectLayer inLayer) const override
	{
		JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
		return m_objectToBroadPhase[inLayer];
	}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
	const char* GetBroadPhaseLayerName(const JPH::BroadPhaseLayer inLayer) const override
	{
		switch (static_cast<JPH::BroadPhaseLayer::Type>(inLayer))
		{
		case static_cast<JPH::BroadPhaseLayer::Type>(BroadPhaseLayers::NON_MOVING):
			return "NON_MOVING";
		case static_cast<JPH::BroadPhaseLayer::Type>(BroadPhaseLayers::MOVING):
			return "MOVING";
		default:
			return "INVALID";
		}
	}
#endif

private:
	JPH::BroadPhaseLayer m_objectToBroadPhase[Layers::NUM_LAYERS];
};

class ObjectVsBroadPhaseLayerFilterImpl final : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
	bool ShouldCollide(const JPH::ObjectLayer inLayer1, const JPH::BroadPhaseLayer inLayer2) const override
	{
		switch (inLayer1)
		{
		case Layers::NON_MOVING:
			return inLayer2 == BroadPhaseLayers::MOVING;
		case Layers::MOVING:
			return true;
		default:
			return false;
		}
	}
};

class ObjectLayerPairFilterImpl final : public JPH::ObjectLayerPairFilter
{
public:
	bool ShouldCollide(const JPH::ObjectLayer inObject1, const JPH::ObjectLayer inObject2) const override
	{
		switch (inObject1)
		{
		case Layers::NON_MOVING:
			return inObject2 == Layers::MOVING;
		case Layers::MOVING:
			return true;
		default:
			return false;
		}
	}
};

class ECSContactListener final : public JPH::ContactListener
{
public:
	mutable std::mutex mtx;
	std::vector<re::physics::CollisionPair> collisions;

	void OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold&, JPH::ContactSettings&) override
	{
		std::lock_guard lock(mtx);
		collisions.push_back({
			static_cast<std::uint32_t>(inBody1.GetUserData()),
			static_cast<std::uint32_t>(inBody2.GetUserData()),
		});
	}
};

} // namespace

namespace re::physics
{

struct JoltPhysicsWorld::JoltState
{
	JPH::PhysicsSystem physicsSystem;
	JPH::TempAllocatorImpl tempAllocator{ 10 * 1024 * 1024 };
	JPH::JobSystemThreadPool jobSystem{ JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, static_cast<int>(std::thread::hardware_concurrency() - 1) };

	BPLayerInterfaceImpl bpLayerInterface;
	ObjectVsBroadPhaseLayerFilterImpl objVsBpFilter;
	ObjectLayerPairFilterImpl objPairFilter;
	ECSContactListener contactListener;
};

void JoltPhysicsWorld::Init()
{
	m_state = std::make_unique<JoltState>();

	constexpr auto MAX_BODIES = 1024;
	constexpr auto MAX_CONTACTS = 1024;

	m_state->physicsSystem.Init(
		MAX_BODIES, 0, MAX_CONTACTS, MAX_BODIES,
		m_state->bpLayerInterface,
		m_state->objVsBpFilter,
		m_state->objPairFilter);

	m_state->physicsSystem.SetContactListener(&m_state->contactListener);

	std::cout << "[Physics] Physics World initialized.\n";
}

void JoltPhysicsWorld::Step(const float deltaTime)
{
	RE_ASSERT(m_state, "JoltState is nullptr. Have you forgot to call JoltPhysicsWorld::Init()?");

	constexpr int collisionSteps = 1;

	m_state->physicsSystem.Update(deltaTime, collisionSteps, &m_state->tempAllocator, &m_state->jobSystem);
}

BodyHandle JoltPhysicsWorld::CreateBody(const RigidBody& desc)
{
	RE_ASSERT(m_state, "JoltState is nullptr. Have you forgot to call JoltPhysicsWorld::Init()?");

	JPH::BodyInterface& bodyInterface = m_state->physicsSystem.GetBodyInterface();

	JPH::RefConst<JPH::Shape> shape;

	// 1. Создаем форму
	switch (desc.collider.type)
	{
	case ColliderType::Box: {
		JPH::Vec3 scaledExtents(
			desc.collider.halfExtends.x * std::abs(desc.scale.x),
			desc.collider.halfExtends.y * std::abs(desc.scale.y),
			desc.collider.halfExtends.z * std::abs(desc.scale.z));
		shape = new JPH::BoxShape(scaledExtents);

		break;
	}
	case ColliderType::Sphere: {
		const float maxScale = std::max({ std::abs(desc.scale.x), std::abs(desc.scale.y), std::abs(desc.scale.z) });
		shape = new JPH::SphereShape(desc.collider.radius * maxScale);

		break;
	}
	case ColliderType::Capsule: {
		const float radiusScale = std::max(std::abs(desc.scale.x), std::abs(desc.scale.z));
		const float heightScale = std::abs(desc.scale.y);
		shape = new JPH::CapsuleShape(desc.collider.height * 0.5f * heightScale, desc.collider.radius * radiusScale);

		break;
	}
	case ColliderType::ConvexMesh: {
		RE_ASSERT(desc.collider.vertices && desc.collider.vertexCount > 0, "No vertices provided for ConvexMesh!");

		JPH::Array<JPH::Vec3> joltVertices;
		joltVertices.reserve(desc.collider.vertexCount);
		for (size_t i = 0; i < desc.collider.vertexCount; ++i)
		{
			// Запекаем скейл прямо в вершины
			joltVertices.push_back(JPH::Vec3(
				desc.collider.vertices[i].x * desc.scale.x,
				desc.collider.vertices[i].y * desc.scale.y,
				desc.collider.vertices[i].z * desc.scale.z));
		}

		JPH::ConvexHullShapeSettings convexSettings(joltVertices);

		shape = convexSettings.Create().Get();
		break;
	}
	case ColliderType::TriangleMesh: {
		RE_ASSERT(desc.collider.vertices && desc.collider.indices, "No data for TriangleMesh!");
		RE_ASSERT(desc.type == BodyType::Static, "TriangleMesh must be static!");

		JPH::VertexList joltVertices;
		joltVertices.reserve(desc.collider.vertexCount);
		for (size_t i = 0; i < desc.collider.vertexCount; ++i)
		{
			joltVertices.push_back(JPH::Float3(
				desc.collider.vertices[i].x * desc.scale.x,
				desc.collider.vertices[i].y * desc.scale.y,
				desc.collider.vertices[i].z * desc.scale.z));
		}

		JPH::IndexedTriangleList joltTriangles;
		joltTriangles.reserve(desc.collider.indexCount / 3);
		for (size_t i = 0; i < desc.collider.indexCount; i += 3)
		{
			joltTriangles.push_back(JPH::IndexedTriangle(desc.collider.indices[i], desc.collider.indices[i + 1], desc.collider.indices[i + 2]));
		}

		JPH::MeshShapeSettings meshSettings(joltVertices, joltTriangles);

		shape = meshSettings.Create().Get();
		break;
	}
	}

	auto motionType = JPH::EMotionType::Dynamic;
	auto layer = Layers::MOVING;

	if (desc.type == BodyType::Static)
	{
		motionType = JPH::EMotionType::Static;
		layer = Layers::NON_MOVING;
	}
	else if (desc.type == BodyType::Kinematic)
	{
		motionType = JPH::EMotionType::Kinematic;
	}

	const float radX = JPH::DegreesToRadians(desc.rotationEuler.x);
	const float radY = JPH::DegreesToRadians(desc.rotationEuler.y);
	const float radZ = JPH::DegreesToRadians(desc.rotationEuler.z);
	const JPH::Quat rotation = JPH::Quat::sEulerAngles(JPH::Vec3(radX, radY, radZ));

	JPH::BodyCreationSettings bodySettings(
		shape,
		JPH::Vec3(desc.position.x, desc.position.y, desc.position.z),
		rotation,
		motionType,
		layer);

	bodySettings.mUserData = desc.entityId;
	bodySettings.mGravityFactor = desc.gravityFactor;
	bodySettings.mIsSensor = desc.isSensor;
	bodySettings.mLinearDamping = desc.linearDamping;

	if (desc.lockRotation)
	{
		bodySettings.mAllowedDOFs = JPH::EAllowedDOFs::TranslationX | JPH::EAllowedDOFs::TranslationY | JPH::EAllowedDOFs::TranslationZ;
	}

	bodySettings.mFriction = desc.friction;
	bodySettings.mRestitution = desc.restitution;

	if (desc.type == BodyType::Dynamic)
	{
		bodySettings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
		bodySettings.mMassPropertiesOverride.mMass = desc.mass;
	}

	const JPH::BodyID bodyID = bodyInterface.CreateAndAddBody(bodySettings, JPH::EActivation::Activate);

	return bodyID.GetIndexAndSequenceNumber();
}

void JoltPhysicsWorld::DestroyBody(const BodyHandle handle)
{
	RE_ASSERT(m_state, "JoltState is nullptr. Have you forgot to call JoltPhysicsWorld::Init()?");

	if (handle == INVALID_BODY_HANDLE)
	{
		return;
	}

	JPH::BodyInterface& bodyInterface = m_state->physicsSystem.GetBodyInterface();
	const JPH::BodyID bodyID(handle);

	bodyInterface.RemoveBody(bodyID);
	bodyInterface.DestroyBody(bodyID);
}

void JoltPhysicsWorld::SetPosition(const BodyHandle handle, const Vector3f& position)
{
	RE_ASSERT(m_state, "JoltState is nullptr. Have you forgot to call JoltPhysicsWorld::Init()?");

	if (handle == INVALID_BODY_HANDLE)
	{
		return;
	}

	JPH::BodyInterface& bodyInterface = m_state->physicsSystem.GetBodyInterface();
	bodyInterface.SetPosition(JPH::BodyID(handle), JPH::Vec3(position.x, position.y, position.z), JPH::EActivation::Activate);
}

Vector3f JoltPhysicsWorld::GetPosition(const BodyHandle handle) const
{
	RE_ASSERT(m_state, "JoltState is nullptr. Have you forgot to call JoltPhysicsWorld::Init()?");

	if (handle == INVALID_BODY_HANDLE)
	{
		return { 0.f, 0.f, 0.f };
	}

	const JPH::BodyInterface& bodyInterface = m_state->physicsSystem.GetBodyInterface();
	const JPH::Vec3 pos = bodyInterface.GetPosition(JPH::BodyID(handle));

	return { pos.GetX(), pos.GetY(), pos.GetZ() };
}

void JoltPhysicsWorld::SetLinearVelocity(const BodyHandle handle, const Vector3f& velocity)
{
	RE_ASSERT(m_state, "JoltState is nullptr. Have you forgot to call JoltPhysicsWorld::Init()?");

	if (handle == INVALID_BODY_HANDLE)
	{
		return;
	}

	JPH::BodyInterface& bodyInterface = m_state->physicsSystem.GetBodyInterface();
	bodyInterface.SetLinearVelocity(JPH::BodyID(handle), JPH::Vec3(velocity.x, velocity.y, velocity.z));

	bodyInterface.ActivateBody(JPH::BodyID(handle));
}

Vector3f JoltPhysicsWorld::GetLinearVelocity(BodyHandle handle) const
{
	RE_ASSERT(m_state, "JoltState is nullptr. Have you forgot to call JoltPhysicsWorld::Init()?");

	if (handle == INVALID_BODY_HANDLE)
	{
		return { 0.f, 0.f, 0.f };
	}

	const JPH::Vec3 vel = m_state->physicsSystem.GetBodyInterface().GetLinearVelocity(JPH::BodyID(handle));
	return { vel.GetX(), vel.GetY(), vel.GetZ() };
}

std::vector<CollisionPair> JoltPhysicsWorld::GetAndClearCollisions()
{
	RE_ASSERT(m_state, "JoltState is nullptr. Have you forgot to call JoltPhysicsWorld::Init()?");

	std::lock_guard lock(m_state->contactListener.mtx);
	auto result = m_state->contactListener.collisions;
	m_state->contactListener.collisions.clear();

	return result;
}

std::unique_ptr<IPhysicsWorld> IPhysicsWorld::Create()
{
	return std::make_unique<JoltPhysicsWorld>();
}

} // namespace re::physics