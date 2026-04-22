#pragma once

#include <RenderCore/Export.hpp>

#include <RenderCore/AnimatedModel.hpp>

#include <glm/glm.hpp>

#include <vector>

namespace re
{

class RE_RENDER_CORE_API Animator
{
public:
	explicit Animator(const AnimatedModel* model);

	// Установить индекс анимации (из массива m_animations модели)
	void PlayAnimation(int animationIndex);

	// Продвинуть время анимации на delta time
	void Update(float dt);

	// Получить готовые матрицы для отправки в шейдер
	const std::vector<glm::mat4>& GetFinalBoneMatrices() const { return m_finalBoneMatrices; }

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