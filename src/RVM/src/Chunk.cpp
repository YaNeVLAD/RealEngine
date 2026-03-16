#include <RVM/Chunk.hpp>

#include <Core/Utils.hpp>

#include <fstream>
#include <iostream>

namespace re::rvm
{

constexpr auto MAGIC_SIGNATURE = "RVM1";
constexpr auto MAGIC_SIGNATURE_SIZE = 4;

enum class ConstTag : uint8_t
{
	Null = 0,
	Int = 1,
	Double = 2,
	String = 3,
};

void Chunk::Write(const std::uint8_t byte)
{
	m_code.push_back(byte);
}

std::uint8_t Chunk::AddConstant(Value const& value)
{
	m_constants.push_back(value);

	return static_cast<std::uint8_t>(m_constants.size() - 1);
}

const std::vector<std::uint8_t>& Chunk::GetCode() const
{
	return m_code;
}

const std::vector<Value>& Chunk::GetConstants() const
{
	return m_constants;
}

std::size_t Chunk::Size() const
{
	return m_code.size();
}

void Chunk::Patch(const std::size_t offset, const std::uint8_t byte)
{
	m_code[offset] = byte;
}

bool Chunk::SaveToFile(String const& filepath) const
{
	std::ofstream out(filepath.ToString(), std::ios::binary);
	if (!out.is_open())
	{
		return false;
	}

	out.write(MAGIC_SIGNATURE, MAGIC_SIGNATURE_SIZE);

	const auto constSize = static_cast<uint32_t>(m_constants.size());
	out.write(reinterpret_cast<const char*>(&constSize), sizeof(constSize));

	for (const auto& val : m_constants)
	{
		std::visit(utils::overloaded{
					   [&out](Null_t) {
						   constexpr auto tag = static_cast<std::uint8_t>(ConstTag::Null);
						   out.write(reinterpret_cast<const char*>(&tag), sizeof(tag));
					   },
					   [&out](const Int v) {
						   constexpr auto tag = static_cast<std::uint8_t>(ConstTag::Int);
						   out.write(reinterpret_cast<const char*>(&tag), sizeof(tag));
						   out.write(reinterpret_cast<const char*>(&v), sizeof(v));
					   },
					   [&out](const Double v) {
						   constexpr auto tag = static_cast<std::uint8_t>(ConstTag::Double);
						   out.write(reinterpret_cast<const char*>(&tag), sizeof(tag));
						   out.write(reinterpret_cast<const char*>(&v), sizeof(v));
					   },
					   [&out](String const& str) {
						   constexpr auto tag = static_cast<std::uint8_t>(ConstTag::String);
						   out.write(reinterpret_cast<const char*>(&tag), sizeof(tag));

						   const std::string stdStr = str.ToString();

						   const auto strLen = static_cast<std::uint32_t>(stdStr.size());
						   out.write(reinterpret_cast<const char*>(&strLen), sizeof(strLen));
						   if (strLen > 0)
						   {
							   out.write(stdStr.data(), strLen);
						   }
					   },
					   [](auto const&) {
						   throw std::runtime_error("Cannot serialize runtime types (Instance/ClassInfo) into Chunk constant pool!");
					   } },
			val);
	}

	const auto codeSize = static_cast<std::uint32_t>(m_code.size());
	out.write(reinterpret_cast<const char*>(&codeSize), sizeof(codeSize));
	out.write(reinterpret_cast<const char*>(m_code.data()), m_code.size());

	return out.good();
}

bool Chunk::LoadFromFile(String const& filepath)
{
	std::ifstream in(filepath.ToString(), std::ios::binary);
	if (!in.is_open())
	{
		return false;
	}

	char magic[MAGIC_SIGNATURE_SIZE + 1] = {};
	in.read(magic, MAGIC_SIGNATURE_SIZE);
	if (std::string(magic) != MAGIC_SIGNATURE)
	{
		std::cerr << "Invalid RVM bytecode file!\n";
		return false;
	}

	std::uint32_t constSize = 0;
	in.read(reinterpret_cast<char*>(&constSize), sizeof(constSize));
	m_constants.clear();
	m_constants.reserve(constSize);

	for (uint32_t i = 0; i < constSize; ++i)
	{
		std::uint8_t tag;
		in.read(reinterpret_cast<char*>(&tag), sizeof(tag));

		switch (static_cast<ConstTag>(tag))
		{
		case ConstTag::Null:
			m_constants.emplace_back(Null);
			break;
		case ConstTag::Int: {
			Int v;
			in.read(reinterpret_cast<char*>(&v), sizeof(v));
			m_constants.emplace_back(v);
			break;
		}
		case ConstTag::Double: {
			Double v;
			in.read(reinterpret_cast<char*>(&v), sizeof(v));
			m_constants.emplace_back(v);
			break;
		}
		case ConstTag::String: {
			std::uint32_t strLen = 0;
			in.read(reinterpret_cast<char*>(&strLen), sizeof(strLen));

			std::string buffer(strLen, '\0');
			if (strLen > 0)
			{
				in.read(buffer.data(), strLen);
			}

			m_constants.emplace_back(String(buffer));
			break;
		}
		default:
			std::cerr << "Unknown constant tag in bytecode!\n";
			return false;
		}
	}

	std::uint32_t codeSize = 0;
	in.read(reinterpret_cast<char*>(&codeSize), sizeof(codeSize));

	m_code.resize(codeSize);
	in.read(reinterpret_cast<char*>(m_code.data()), codeSize);

	return in.good();
}

} // namespace re::rvm