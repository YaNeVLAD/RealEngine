#include <RenderCore/GLFW/Shader.hpp>

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <iostream>

namespace
{

std::string ReadFile(re::String const& filepath)
{
	const std::ifstream file(filepath.ToString());
	if (!file.is_open())
	{
		std::cerr << "Shader::ReadFile: Failed to open file: " << filepath << std::endl;
		return "";
	}
	std::stringstream buffer;
	buffer << file.rdbuf();

	return buffer.str();
}

} // namespace

namespace re::render
{

Shader::Shader(String const& computeSrc)
{
	const std::uint32_t computeShader = CompileShader(GL_COMPUTE_SHADER, computeSrc);

	m_rendererId = glCreateProgram();
	glAttachShader(m_rendererId, computeShader);
	glLinkProgram(m_rendererId);

	int isLinked = 0;
	glGetProgramiv(m_rendererId, GL_LINK_STATUS, &isLinked);
	if (isLinked == GL_FALSE)
	{
		int maxLength = 0;
		glGetProgramiv(m_rendererId, GL_INFO_LOG_LENGTH, &maxLength);
		std::vector<char> infoLog(maxLength);
		glGetProgramInfoLog(m_rendererId, maxLength, &maxLength, &infoLog[0]);

		std::cerr << "Compute Shader linking failed:\n"
				  << infoLog.data() << std::endl;

		glDeleteProgram(m_rendererId);
		glDeleteShader(computeShader);
		return;
	}

	glDetachShader(m_rendererId, computeShader);
	glDeleteShader(computeShader);
}

Shader::Shader(file_system::ShadersPath const& computePath)
	: m_computePath(computePath.Str())
	, m_isCompute(true)
{
	Reload();
}

Shader::Shader(String const& vertexSrc, String const& fragmentSrc)
	: m_isCompute(false)
{
	const std::uint32_t vertexShader = CompileShader(GL_VERTEX_SHADER, vertexSrc);
	const std::uint32_t fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentSrc);

	m_rendererId = glCreateProgram();
	glAttachShader(m_rendererId, vertexShader);
	glAttachShader(m_rendererId, fragmentShader);
	glLinkProgram(m_rendererId);

	int isLinked = 0;
	glGetProgramiv(m_rendererId, GL_LINK_STATUS, &isLinked);
	if (isLinked == GL_FALSE)
	{
		int maxLength = 0;
		glGetProgramiv(m_rendererId, GL_INFO_LOG_LENGTH, &maxLength);
		std::vector<char> infoLog(maxLength);
		glGetProgramInfoLog(m_rendererId, maxLength, &maxLength, &infoLog[0]);

		std::cerr << "Shader linking failed:\n"
				  << infoLog.data() << std::endl;

		glDeleteProgram(m_rendererId);
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
		return;
	}

	glDetachShader(m_rendererId, vertexShader);
	glDetachShader(m_rendererId, fragmentShader);
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
}

Shader::Shader(file_system::ShadersPath&& vertexPath, file_system::ShadersPath&& fragmentPath)
	: m_vertexPath(vertexPath.Str())
	, m_fragmentPath(fragmentPath.Str())
	, m_isCompute(false)
{
	Reload();
}

Shader::~Shader()
{
	if (m_rendererId != 0)
	{
		glDeleteProgram(m_rendererId);
	}
}

std::uint32_t Shader::CompileShader(const std::uint32_t type, String const& source)
{
	const std::uint32_t shader = glCreateShader(type);
	const auto u8Source = source.ToString();
	const auto src = u8Source.c_str();
	glShaderSource(shader, 1, &src, nullptr);
	glCompileShader(shader);

	int isCompiled = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
	if (isCompiled == GL_FALSE)
	{
		int maxLength = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
		std::vector<char> infoLog(maxLength);
		glGetShaderInfoLog(shader, maxLength, &maxLength, &infoLog[0]);

		std::cerr << "Shader compilation failed:\n"
				  << infoLog.data() << std::endl;
		glDeleteShader(shader);

		return 0;
	}

	return shader;
}

void Shader::Bind() const
{
	glUseProgram(m_rendererId);
}

void Shader::Unbind() const
{
	glUseProgram(0);
}

void* Shader::GetNativeHandle() const
{
	return (void*)&m_rendererId;
}

bool Shader::Reload()
{
	const std::uint32_t newProgramId = glCreateProgram();

	if (m_isCompute)
	{
		const std::string computeSrc = ReadFile(m_computePath);
		if (computeSrc.empty())
		{
			glDeleteProgram(newProgramId);
			return false;
		}

		const std::uint32_t computeShader = CompileShader(GL_COMPUTE_SHADER, computeSrc);
		if (computeShader == 0)
		{
			glDeleteProgram(newProgramId);
			return false;
		}

		glAttachShader(newProgramId, computeShader);
		glLinkProgram(newProgramId);

		int isLinked = 0;
		glGetProgramiv(newProgramId, GL_LINK_STATUS, &isLinked);
		if (isLinked == GL_FALSE)
		{
			int maxLength = 0;
			glGetProgramiv(newProgramId, GL_INFO_LOG_LENGTH, &maxLength);
			std::vector<char> infoLog(maxLength);
			glGetProgramInfoLog(newProgramId, maxLength, &maxLength, &infoLog[0]);

			std::cerr << "Compute Shader linking failed (" << m_computePath << "):\n"
					  << infoLog.data() << std::endl;
			glDeleteProgram(newProgramId);
			glDeleteShader(computeShader);
			return false;
		}

		glDetachShader(newProgramId, computeShader);
		glDeleteShader(computeShader);
	}
	else
	{
		const std::string vertexSrc = ReadFile(m_vertexPath);
		const std::string fragmentSrc = ReadFile(m_fragmentPath);

		if (vertexSrc.empty() || fragmentSrc.empty())
		{
			glDeleteProgram(newProgramId);

			return false;
		}

		const std::uint32_t vertexShader = CompileShader(GL_VERTEX_SHADER, vertexSrc);
		const std::uint32_t fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentSrc);

		if (vertexShader == 0 || fragmentShader == 0)
		{
			if (vertexShader != 0)
			{
				glDeleteShader(vertexShader);
			}
			if (fragmentShader != 0)
			{
				glDeleteShader(fragmentShader);
			}
			glDeleteProgram(newProgramId);

			return false;
		}

		glAttachShader(newProgramId, vertexShader);
		glAttachShader(newProgramId, fragmentShader);
		glLinkProgram(newProgramId);

		int isLinked = 0;
		glGetProgramiv(newProgramId, GL_LINK_STATUS, &isLinked);
		if (isLinked == GL_FALSE)
		{
			int maxLength = 0;
			glGetProgramiv(newProgramId, GL_INFO_LOG_LENGTH, &maxLength);
			std::vector<char> infoLog(maxLength);
			glGetProgramInfoLog(newProgramId, maxLength, &maxLength, &infoLog[0]);

			std::cerr << "Shader linking failed ("
					  << m_vertexPath << ", " << m_fragmentPath << "):\n"
					  << infoLog.data() << std::endl;
			glDeleteProgram(newProgramId);
			glDeleteShader(vertexShader);
			glDeleteShader(fragmentShader);
			return false;
		}

		glDetachShader(newProgramId, vertexShader);
		glDetachShader(newProgramId, fragmentShader);
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
	}

	if (m_rendererId != 0)
	{
		glDeleteProgram(m_rendererId);
	}

	m_rendererId = newProgramId;
	m_uniformLocationCache.clear();

	std::cout << "Shader reloaded successfully!" << std::endl;
	return true;
}

int Shader::GetUniformLocation(const std::string_view name)
{
	std::string nameStr(name);
	if (m_uniformLocationCache.contains(nameStr))
	{
		return m_uniformLocationCache[nameStr];
	}

	const int location = glGetUniformLocation(m_rendererId, name.data());
	if (location == -1)
	{
		std::cerr << "Warning: uniform '" << name << "' doesn't exist!" << std::endl;
	}

	m_uniformLocationCache[nameStr] = location;

	return location;
}

void Shader::SetInt(const std::string_view name, const int value)
{
	glUniform1i(GetUniformLocation(name), value);
}

void Shader::SetFloat(const std::string_view name, const float value)
{
	glUniform1f(GetUniformLocation(name), value);
}

void Shader::SetFloat3(const std::string_view name, const glm::vec3& value)
{
	glUniform3f(GetUniformLocation(name), value.x, value.y, value.z);
}

void Shader::SetFloat4(const std::string_view name, const glm::vec4& value)
{
	glUniform4f(GetUniformLocation(name), value.x, value.y, value.z, value.w);
}

void Shader::SetMat4(const std::string_view name, const glm::mat4& value)
{
	glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::SetUInt(const std::string_view name, const std::uint32_t value)
{
	glUniform1ui(GetUniformLocation(name), value);
}

void Shader::SetBool(const std::string_view name, const bool value)
{
	glUniform1i(GetUniformLocation(name), value);
}

void Shader::SetFloat4Array(const std::string_view name, const glm::vec4* values, const std::uint32_t count)
{
	glUniform4fv(GetUniformLocation(name), static_cast<int>(count), glm::value_ptr(values[0]));
}

} // namespace re::render
