#pragma once

#include <Core/Math/Color.hpp>
#include <Core/Math/Vector3.hpp>

#include <glm/glm.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string_view>
#include <vector>

namespace re
{
class Texture;
}

namespace re::render
{

struct RenderEntityHandle
{
	std::uint64_t id = 0;
	[[nodiscard]] explicit operator bool() const noexcept { return id != 0; }
	bool operator==(const RenderEntityHandle& other) const = default;
};

struct EntityRenderHandles
{
	RenderEntityHandle meshHandle{};
	RenderEntityHandle lightHandle{};
	std::vector<RenderEntityHandle> animatedMeshHandles{};

	[[nodiscard]] bool IsEmpty() const
	{
		return !meshHandle && !lightHandle && animatedMeshHandles.empty();
	}
};

struct MeshDataView
{
	const void* vertices = nullptr;
	std::size_t vertexCount = 0;
	std::size_t vertexStride = 0;
	const void* indices = nullptr;
	std::size_t indexCount = 0;
	bool is32BitIndices = true;
};

struct MaterialDataView
{
	Color ambient = Color::White;
	Color diffuse = Color::White;
	Color specular = Color::White;
	float shininess = 32.0f;
	std::uint32_t albedoTextureID = 0;
	const Texture* albedoTexture = nullptr;
	std::string_view materialName;
};

struct SkinDataView
{
	const glm::mat4* bones = nullptr;
	std::size_t boneCount = 0;
	std::size_t boneIndicesOffset = 0;
	std::size_t boneWeightsOffset = 0;
};

struct LightDataView
{
	enum class LightType : int
	{
		Directional,
		Light,
		Spot
	};

	LightType type = LightType::Directional;
	glm::vec3 position{ 0.0f };
	glm::vec3 direction{ 0.0f, -1.0f, 0.0f };
	glm::vec3 color{ 1.0f };
	float intensity = 1.0f;
	float cutOffAngle = 45.0f;
	float falloff = 150.0f;
};

struct CameraDataView
{
	Vector3f position{ 0.0f };
	glm::mat4 viewMatrix{ 1.0f };
	float fov = 45.0f;
	float aspect = 1.777f;
	float nearClip = 0.1f;
	float farClip = 1000.0f;
	float iso = 100.0f;
};

struct UIDrawData
{
	void* nativeData = nullptr;
};

class IRenderBackend
{
public:
	virtual ~IRenderBackend() = default;

	virtual void Init(void* windowHandle, std::uint32_t width, std::uint32_t height) = 0;
	virtual void Shutdown() = 0;

	virtual void Resize(std::uint32_t width, std::uint32_t height) = 0;

	virtual RenderEntityHandle CreateStaticMesh(const MeshDataView& mesh, const MaterialDataView& material) = 0;
	virtual RenderEntityHandle CreateAnimatedMesh(const MeshDataView& mesh, const MaterialDataView& material, const SkinDataView& skin) = 0;
	virtual RenderEntityHandle CreateLight(const LightDataView& light) = 0;
	virtual void DestroyEntity(RenderEntityHandle handle) = 0;

	virtual void UpdateTransform(RenderEntityHandle handle, const glm::mat4& transformMatrix) = 0;
	virtual void UpdateBones(RenderEntityHandle handle, const glm::mat4* bones, std::size_t boneCount) = 0;
	virtual void UpdateLight(RenderEntityHandle handle, const LightDataView& light) = 0;
	virtual void UpdateCamera(const CameraDataView& camera) = 0;
	virtual void SetSkybox(std::uint32_t cubemapID, std::uint32_t irradianceID) = 0;

	virtual void SetAmbientLight(float intensity, Color color) = 0;

	virtual void SetClearColor(Color color) = 0;

	virtual void RenderUI(float dt, std::function<void()> uiCallback) = 0;

	virtual void BeginFrame() = 0;
	virtual void RenderFrame() = 0;
	virtual void EndFrame() = 0;
};

} // namespace re::render