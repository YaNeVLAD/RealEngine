#pragma once

#include <RenderCore/Export.hpp>

#include <RenderCore/AnimatedModel.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>

#include <vector>

namespace re
{

class RE_RENDER_CORE_API Animator
{
public:
	explicit Animator(const AnimatedModel* model);

	void PlayAnimation(int animationIndex);

	void Update(float dt);

	const std::vector<glm::mat4>& FinalBoneMatrices() const { return m_finalBoneMatrices; }

private:
	void CalculateBoneTransform(int boneIndex, const glm::mat4& parentTransform);

	static glm::vec3 InterpolatePosition(float animationTime, const render::BoneAnimation& boneAnim);
	static glm::quat InterpolateRotation(float animationTime, const render::BoneAnimation& boneAnim);
	static glm::vec3 InterpolateScale(float animationTime, const render::BoneAnimation& boneAnim);

private:
	const AnimatedModel* m_model = nullptr;
	int m_currentAnimationIndex = -1;
	float m_currentTime = 0.0f;

	std::vector<glm::mat4> m_finalBoneMatrices;
};

} // namespace re