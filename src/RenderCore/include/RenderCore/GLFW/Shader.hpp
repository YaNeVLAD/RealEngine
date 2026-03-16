#pragma once

#include <Core/String.hpp>
#include <RenderCore/Export.hpp>

#include <glm/glm.hpp>

#include <string_view>
#include <unordered_map>

namespace re::render
{

class RE_RENDER_CORE_API Shader
{

public:
	Shader(String const& vertexSrc, String const& fragmentSrc);
	~Shader();

	void Bind() const;
	void Unbind() const;

	void SetInt(std::string_view name, int value);
	void SetFloat(std::string_view name, float value);
	void SetFloat3(std::string_view name, const glm::vec3& value);
	void SetFloat4(std::string_view name, const glm::vec4& value);
	void SetMat4(std::string_view name, const glm::mat4& value);

private:
	std::uint32_t CompileShader(std::uint32_t type, String const& source);
	int GetUniformLocation(std::string_view name);

private:
	std::uint32_t m_rendererId;
	std::unordered_map<std::string, int, std::hash<std::string_view>, std::equal_to<>> m_uniformLocationCache;
};

} // namespace re::render
