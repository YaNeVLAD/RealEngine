#pragma once

#include <Core/Math/Color.hpp>
#include <Core/Math/Vector2.hpp>
#include <Core/Math/Vector3.hpp>

#include <glm/glm.hpp>

#include <cstdint>

namespace re::render
{

template <typename T>
struct BufferTypeTraits;

#define DECLARE_BUFFER_TRAIT(Type, ComponentCount, BaseTypeID) \
	template <>                                                \
	struct BufferTypeTraits<Type>                              \
	{                                                          \
		static constexpr std::uint32_t Count = ComponentCount; \
		static constexpr std::uint32_t BaseType = BaseTypeID;  \
	}

constexpr std::uint32_t Type_Float = 0x1406; // GL_FLOAT
constexpr std::uint32_t Type_Int = 0x1404; // GL_INT
constexpr std::uint32_t Type_UnsignedByte = 0x1401; // GL_UNSIGNED_BYTE
constexpr std::uint32_t Type_Bool = 0x8B56; // GL_BOOL

// clang-format off
DECLARE_BUFFER_TRAIT(float,      1, Type_Float);
DECLARE_BUFFER_TRAIT(glm::vec2,  2, Type_Float);
DECLARE_BUFFER_TRAIT(glm::vec3,  3, Type_Float);
DECLARE_BUFFER_TRAIT(glm::vec4,  4, Type_Float);

DECLARE_BUFFER_TRAIT(int,        1, Type_Int);
DECLARE_BUFFER_TRAIT(glm::ivec2, 2, Type_Int);
DECLARE_BUFFER_TRAIT(glm::ivec3, 3, Type_Int);
DECLARE_BUFFER_TRAIT(glm::ivec4, 4, Type_Int);

DECLARE_BUFFER_TRAIT(Vector2f, 2, Type_Float);
DECLARE_BUFFER_TRAIT(Vector2i, 2, Type_Int);

DECLARE_BUFFER_TRAIT(Vector3f, 3, Type_Float);
DECLARE_BUFFER_TRAIT(Vector3i, 3, Type_Int);

DECLARE_BUFFER_TRAIT(Color, 4, Type_UnsignedByte);
// clang-format on

#undef DECLARE_BUFFER_TRAIT

} // namespace re::render