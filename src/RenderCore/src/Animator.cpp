#include <RenderCore/Animator.hpp>

#include <glm/gtx/quaternion.hpp>

namespace re
{

Animator::Animator(const AnimatedModel* model)
	: m_model(model)
{
	if (m_model)
	{
		m_finalBoneMatrices.resize(m_model->Skeleton().size(), glm::mat4(1.0f));
	}
}

void Animator::PlayAnimation(const int animationIndex)
{
	if (!m_model || animationIndex < 0 || animationIndex >= m_model->Animations().size())
	{
		return;
	}
	m_currentAnimationIndex = animationIndex;
	m_currentTime = 0.0f;
}

void Animator::Update(const float dt)
{
	if (!m_model || m_currentAnimationIndex < 0)
	{
		return;
	}

	const render::Animation& animation = m_model->Animations()[m_currentAnimationIndex];

	m_currentTime += dt;
	if (animation.duration > 0.0f)
	{
		m_currentTime = std::fmod(m_currentTime, animation.duration);
	}

	const auto& skeleton = m_model->Skeleton();

	for (std::size_t i = 0; i < skeleton.size(); ++i)
	{
		if (skeleton[i].parentIndex == render::Bone::ROOT_BONE_IDX)
		{
			CalculateBoneTransform(static_cast<int>(i), glm::mat4(1.0f));
		}
	}
}

void Animator::CalculateBoneTransform(const int boneIndex, const glm::mat4& parentTransform)
{
	using namespace re::render;

	const auto& skeleton = m_model->Skeleton();
	const Bone& bone = skeleton[boneIndex];
	const Animation& animation = m_model->Animations()[m_currentAnimationIndex];

	glm::mat4 localTransform = bone.localTransform; // Берем базовый трансформ из T-позы

	// Если для этой кости есть анимация - переопределяем локальный трансформ
	if (animation.boneAnimations.contains(boneIndex))
	{
		const BoneAnimation& boneAnim = animation.boneAnimations.at(boneIndex);

		const glm::vec3 position = InterpolatePosition(m_currentTime, boneAnim);
		const glm::quat rotation = InterpolateRotation(m_currentTime, boneAnim);
		const glm::vec3 scale = InterpolateScale(m_currentTime, boneAnim);

		localTransform = glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(rotation) * glm::scale(glm::mat4(1.0f), scale);
	}

	// Глобальная матрица = глобальная матрица родителя * наша локальная
	const glm::mat4 globalTransform = parentTransform * localTransform;

	// Итоговая матрица для шейдера: Глобальная * Inverse Bind Matrix
	m_finalBoneMatrices[boneIndex] = globalTransform * bone.inverseBindMatrix;

	// Рекурсивно вызываем для всех детей
	for (std::size_t i = 0; i < skeleton.size(); ++i)
	{
		if (skeleton[i].parentIndex == boneIndex)
		{
			CalculateBoneTransform(static_cast<int>(i), globalTransform);
		}
	}
}

glm::vec3 Animator::InterpolatePosition(const float animationTime, const render::BoneAnimation& boneAnim)
{
	const auto& keys = boneAnim.positions;
	if (keys.empty())
	{
		return glm::vec3(0.0f);
	}
	if (keys.size() == 1)
	{
		return keys[0].value;
	}

	for (std::size_t i = 0; i < keys.size() - 1; ++i)
	{
		if (animationTime < keys[i + 1].time)
		{
			const float scaleFactor = (animationTime - keys[i].time) / (keys[i + 1].time - keys[i].time);

			return glm::mix(keys[i].value, keys[i + 1].value, scaleFactor);
		}
	}

	return keys.back().value;
}

glm::quat Animator::InterpolateRotation(const float animationTime, const render::BoneAnimation& boneAnim)
{
	const auto& keys = boneAnim.rotations;
	if (keys.empty())
	{
		return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	}
	if (keys.size() == 1)
	{
		return keys[0].value;
	}

	for (std::size_t i = 0; i < keys.size() - 1; ++i)
	{
		if (animationTime < keys[i + 1].time)
		{
			const float scaleFactor = (animationTime - keys[i].time) / (keys[i + 1].time - keys[i].time);

			return glm::normalize(glm::slerp(keys[i].value, keys[i + 1].value, scaleFactor));
		}
	}

	return keys.back().value;
}

glm::vec3 Animator::InterpolateScale(const float animationTime, const render::BoneAnimation& boneAnim)
{
	const auto& keys = boneAnim.scales;
	if (keys.empty())
	{
		return glm::vec3(1.0f);
	}
	if (keys.size() == 1)
	{
		return keys[0].value;
	}

	for (std::size_t i = 0; i < keys.size() - 1; ++i)
	{
		if (animationTime < keys[i + 1].time)
		{
			const float scaleFactor = (animationTime - keys[i].time) / (keys[i + 1].time - keys[i].time);

			return glm::mix(keys[i].value, keys[i + 1].value, scaleFactor);
		}
	}

	return keys.back().value;
}

} // namespace re