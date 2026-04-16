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

JPH::Vec3 ToJPH(const re::Vector3f& v)
{
	return { v.x, v.y, v.z };
}

re::Vector3f FromJPH(const JPH::Vec3& v)
{
	return { v.GetX(), v.GetY(), v.GetZ() };
}

JPH::RefConst<JPH::Shape> CreateShape(const re::physics::RigidBody& desc)
{
	using namespace re::physics;

	switch (desc.collider.type)
	{
	case ColliderType::Box: {
		const JPH::Vec3 scaledExtents(
			desc.collider.halfExtends.x * std::abs(desc.scale.x),
			desc.collider.halfExtends.y * std::abs(desc.scale.y),
			desc.collider.halfExtends.z * std::abs(desc.scale.z));
		return new JPH::BoxShape(scaledExtents);
	}
	case ColliderType::Sphere: {
		const float maxScale = std::max({ std::abs(desc.scale.x), std::abs(desc.scale.y), std::abs(desc.scale.z) });
		return new JPH::SphereShape(desc.collider.radius * maxScale);
	}
	case ColliderType::Capsule: {
		const float radiusScale = std::max(std::abs(desc.scale.x), std::abs(desc.scale.z));
		const float heightScale = std::abs(desc.scale.y);
		return new JPH::CapsuleShape(desc.collider.height * 0.5f * heightScale, desc.collider.radius * radiusScale);
	}
	case ColliderType::ConvexMesh: {
		RE_ASSERT(desc.collider.vertices && desc.collider.vertexCount > 0, "No vertices provided for ConvexMesh!");

		JPH::Array<JPH::Vec3> joltVertices;
		joltVertices.reserve(desc.collider.vertexCount);
		for (size_t i = 0; i < desc.collider.vertexCount; ++i)
		{
			joltVertices.push_back(ToJPH(desc.collider.vertices[i] * desc.scale));
		}
		return JPH::ConvexHullShapeSettings(joltVertices).Create().Get();
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
		for (size_t i = 0; i < desc.collider.indexCount; i += 3)
		{
			joltTriangles.push_back({ desc.collider.indices[i], desc.collider.indices[i + 1], desc.collider.indices[i + 2] });
		}
		return JPH::MeshShapeSettings(joltVertices, joltTriangles).Create().Get();
	}
	default:
		return nullptr;
	}
}

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

	const auto shape = CreateShape(desc);
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

	const JPH::Quat rotation = JPH::Quat::sEulerAngles(ToJPH(desc.rotationEuler) * (std::numbers::phi_v<float> / 180.0f));

	JPH::BodyCreationSettings settings(shape, ToJPH(desc.position), rotation, motionType, layer);

	settings.mUserData = desc.entityId;
	settings.mGravityFactor = desc.gravityFactor;
	settings.mIsSensor = desc.isSensor;
	settings.mLinearDamping = desc.linearDamping;
	settings.mFriction = desc.friction;
	settings.mRestitution = desc.restitution;

	auto degreesOfFreedom = JPH::EAllowedDOFs::All;

	// clang-format off
	if (desc.lockTranslation.x) degreesOfFreedom &= ~JPH::EAllowedDOFs::TranslationX;
	if (desc.lockTranslation.y) degreesOfFreedom &= ~JPH::EAllowedDOFs::TranslationY;
	if (desc.lockTranslation.z) degreesOfFreedom &= ~JPH::EAllowedDOFs::TranslationZ;

	if (desc.lockRotation.x) degreesOfFreedom &= ~JPH::EAllowedDOFs::RotationX;
	if (desc.lockRotation.y) degreesOfFreedom &= ~JPH::EAllowedDOFs::RotationY;
	if (desc.lockRotation.z) degreesOfFreedom &= ~JPH::EAllowedDOFs::RotationZ;
	// clang-format on
	settings.mAllowedDOFs = degreesOfFreedom;

	if (desc.type == BodyType::Dynamic)
	{
		settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
		settings.mMassPropertiesOverride.mMass = desc.mass;
	}

	return m_state->physicsSystem.GetBodyInterface().CreateAndAddBody(settings, JPH::EActivation::Activate).GetIndexAndSequenceNumber();
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

	m_state->physicsSystem.GetBodyInterface().SetPosition(JPH::BodyID(handle), ToJPH(position), JPH::EActivation::Activate);
}

Vector3f JoltPhysicsWorld::GetPosition(const BodyHandle handle) const
{
	RE_ASSERT(m_state, "JoltState is nullptr. Have you forgot to call JoltPhysicsWorld::Init()?");

	if (handle == INVALID_BODY_HANDLE)
	{
		return { 0.f, 0.f, 0.f };
	}

	return FromJPH(m_state->physicsSystem.GetBodyInterface().GetPosition(JPH::BodyID(handle)));
}

void JoltPhysicsWorld::SetLinearVelocity(const BodyHandle handle, const Vector3f& velocity)
{
	RE_ASSERT(m_state, "JoltState is nullptr. Have you forgot to call JoltPhysicsWorld::Init()?");

	if (handle == INVALID_BODY_HANDLE)
	{
		return;
	}

	JPH::BodyInterface& bodyInterface = m_state->physicsSystem.GetBodyInterface();

	bodyInterface.SetLinearVelocity(JPH::BodyID(handle), ToJPH(velocity));
	bodyInterface.ActivateBody(JPH::BodyID(handle));
}

Vector3f JoltPhysicsWorld::GetLinearVelocity(const BodyHandle handle) const
{
	RE_ASSERT(m_state, "JoltState is nullptr. Have you forgot to call JoltPhysicsWorld::Init()?");

	if (handle == INVALID_BODY_HANDLE)
	{
		return { 0.f, 0.f, 0.f };
	}

	return FromJPH(m_state->physicsSystem.GetBodyInterface().GetLinearVelocity(JPH::BodyID(handle)));
}

void JoltPhysicsWorld::UpdateScale(const BodyHandle handle, const RigidBody& desc)
{
	RE_ASSERT(m_state, "JoltState is nullptr");
	if (handle == INVALID_BODY_HANDLE)
	{
		return;
	}

	const auto shape = CreateShape(desc);
	if (shape)
	{
		m_state->physicsSystem.GetBodyInterface().SetShape(JPH::BodyID(handle), shape, false, JPH::EActivation::Activate);
	}
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