#pragma once

#include <RenderCore/Export.hpp>

#include <RenderCore/Assets/IAsset.hpp>
#include <RenderCore/Internal/MeshPart.hpp>
#include <RenderCore/Vertex.hpp>

#include <glm/ext/quaternion_float.hpp>
#include <glm/mat4x4.hpp>

#include <unordered_map>
#include <vector>

namespace re
{

namespace render
{

struct Bone
{
	constexpr static int ROOT_BONE_IDX = -1;

	std::string name;
	int parentIndex = ROOT_BONE_IDX;

	glm::mat4 inverseBindMatrix{ 1.0f };

	glm::mat4 localTransform{ 1.0f };
	glm::mat4 globalTransform{ 1.0f };

	glm::vec3 translation{ 0.0f };
	glm::quat rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
	glm::vec3 scale{ 1.0f };
};

struct PositionKeyframe
{
	float time;
	glm::vec3 value;
};

struct RotationKeyframe
{
	float time;
	glm::quat value;
};

struct ScaleKeyframe
{
	float time;
	glm::vec3 value;
};

struct BoneAnimation
{
	std::vector<PositionKeyframe> positions;
	std::vector<RotationKeyframe> rotations;
	std::vector<ScaleKeyframe> scales;
};

struct Animation
{
	std::string name;
	float duration = 0.0f;
	std::unordered_map<int, BoneAnimation> boneAnimations;
};

} // namespace render

class RE_RENDER_CORE_API AnimatedModel final : public IAsset
{
public:
	[[nodiscard]] const std::vector<render::MeshPart>& Parts() const;
	[[nodiscard]] const std::vector<render::Bone>& Skeleton() const;
	[[nodiscard]] const std::vector<render::Animation>& Animations() const;

	bool LoadFromFile(const String& filePath, const AssetManager* manager) override;

private:
	std::vector<render::MeshPart> m_parts;
	std::vector<render::Bone> m_skeleton;
	std::vector<render::Animation> m_animations;
};

} // namespace re