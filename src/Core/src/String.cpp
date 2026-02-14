#include <Core/String.hpp>

#include <utility>

namespace re
{

String::String(const char32_t* u32Str)
	: m_string(u32Str ? u32Str : U"")
{
}

String::String(std::u32string u32Str)
	: m_string(std::move(u32Str))
{
}

String::String(const char* u8Str)
	: m_string(u8Str ? DecodeUtf8(u8Str) : U"")
{
}

String::String(const std::string_view u8Str)
	: m_string(DecodeUtf8(u8Str))
{
}

String::String(std::string const& u8Str)
	: m_string(DecodeUtf8(u8Str))
{
}

String::String(const wchar_t* wStr)
	: m_string(wStr ? DecodeWide(wStr) : U"")
{
}

String::String(std::wstring_view wStr)
	: m_string(DecodeWide(wStr))
{
}

String::String(std::wstring const& wStr)
	: m_string(DecodeWide(wStr))
{
}

String::operator std::string() const
{
	return EncodeUtf8(m_string);
}

String::operator std::wstring() const
{
	return EncodeWide(m_string);
}

std::string String::ToString() const
{
	return EncodeUtf8(m_string);
}

std::wstring String::ToWString() const
{
	return EncodeWide(m_string);
}

std::u32string String::ToU32String() const
{
	return m_string;
}

const char32_t* String::Data() const
{
	return m_string.data();
}

String::Iterator String::begin()
{
	return m_string.begin();
}

String::ConstIterator String::begin() const
{
	return m_string.begin();
}

String::Iterator String::end()
{
	return m_string.end();
}

String::ConstIterator String::end() const
{
	return m_string.end();
}

std::size_t String::Length() const
{
	return m_string.length();
}

std::size_t String::Size() const
{
	return m_string.size();
}

bool String::Empty() const
{
	return m_string.empty();
}

void String::Reserve(const std::size_t newCap)
{
	m_string.reserve(newCap);
}

void String::Clear()
{
	m_string.clear();
}

void String::Resize(const std::size_t count)
{
	m_string.resize(count);
}

char32_t& String::operator[](const std::size_t pos)
{
	return m_string[pos];
}

const char32_t& String::operator[](const std::size_t pos) const
{
	return m_string[pos];
}

void String::Append(const String& other)
{
	m_string.append(other.m_string);
}

void String::PushBack(const char32_t ch)
{
	m_string.push_back(ch);
}

std::size_t String::Find(const String& str, const std::size_t pos) const
{
	return m_string.find(str.m_string, pos);
}

String String::Substring(const std::size_t pos, const std::size_t count) const
{
	return { m_string.substr(pos, count) };
}

String& String::operator+=(const String& other)
{
	m_string += other.m_string;
	return *this;
}

std::u32string String::DecodeUtf8(const std::string_view u8Str)
{
	std::u32string result;
	result.reserve(u8Str.size());

	for (std::size_t i = 0; i < u8Str.size(); ++i)
	{
		auto c = static_cast<unsigned char>(u8Str[i]);

		if (c < 0x80)
		{
			result.push_back(c);
		}
		else if ((c & 0xE0) == 0xC0)
		{
			if (i + 1 < u8Str.size())
			{
				char32_t cp = (c & 0x1F) << 6;
				cp |= static_cast<unsigned char>(u8Str[++i] & 0x3F);
				result.push_back(cp);
			}
		}
		else if ((c & 0xF0) == 0xE0)
		{
			if (i + 2 < u8Str.size())
			{
				char32_t cp = (c & 0x0F) << 12;
				cp |= static_cast<unsigned char>(u8Str[++i] & 0x3F) << 6;
				cp |= static_cast<unsigned char>(u8Str[++i] & 0x3F);
				result.push_back(cp);
			}
		}
		else if ((c & 0xF8) == 0xF0)
		{
			if (i + 3 < u8Str.size())
			{
				char32_t cp = (c & 0x07) << 18;
				cp |= static_cast<unsigned char>(u8Str[++i] & 0x3F) << 12;
				cp |= static_cast<unsigned char>(u8Str[++i] & 0x3F) << 6;
				cp |= static_cast<unsigned char>(u8Str[++i] & 0x3F);
				result.push_back(cp);
			}
		}
	}

	return result;
}

std::string String::EncodeUtf8(std::u32string const& u32Str)
{
	std::string result;
	result.reserve(u32Str.size() * 4);

	for (const char32_t cp : u32Str)
	{
		if (cp < 0x80)
		{
			result.push_back(static_cast<char>(cp));
		}
		else if (cp < 0x800)
		{
			result.push_back(static_cast<char>(0xC0 | (cp >> 6)));
			result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
		}
		else if (cp < 0x10000)
		{
			result.push_back(static_cast<char>(0xE0 | (cp >> 12)));
			result.push_back(static_cast<char>(0x80 | (cp >> 6) & 0x3F));
			result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
		}
		else
		{
			result.push_back(static_cast<char>(0xF0 | (cp >> 18)));
			result.push_back(static_cast<char>(0x80 | (cp >> 12) & 0x3F));
			result.push_back(static_cast<char>(0x80 | (cp >> 6) & 0x3F));
			result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
		}
	}

	return result;
}

std::u32string String::DecodeWide(std::wstring_view wStr)
{
	if constexpr (sizeof(wchar_t) == sizeof(char32_t))
	{ // For linux
		return std::u32string(wStr.begin(), wStr.end());
	}

	std::u32string result;
	result.reserve(wStr.size());

	for (std::size_t i = 0; i < wStr.size(); ++i)
	{
		const auto wc = static_cast<char32_t>(wStr[i]);

		if (wc >= 0xD800 && wc <= 0xDBFF)
		{
			if (i + 1 < wStr.size())
			{
				const auto low = static_cast<char32_t>(wStr[++i]);
				if (low >= 0xDC00 && low <= 0xDFFF)
				{
					const char32_t cp = ((wc - 0xD800) << 10) + (low - 0xDC00) + 0x10000;
					result.push_back(cp);
				}
			}
			else
			{
				result.push_back(wc);
			}
		}
	}

	return result;
}

std::wstring String::EncodeWide(std::u32string const& u32Str)
{
	if constexpr (sizeof(wchar_t) == sizeof(char32_t))
	{ // For linux
		return std::wstring(u32Str.begin(), u32Str.end());
	}

	std::wstring result;
	result.reserve(u32Str.size());

	for (char32_t cp : u32Str)
	{
		if (cp <= 0x10000)
		{
			result.push_back(static_cast<wchar_t>(cp));
		}
		else
		{
			cp -= 0x10000;
			result.push_back(static_cast<wchar_t>(0xD800 | (cp >> 10)));
			result.push_back(static_cast<wchar_t>(0xDC00 | (cp & 0x3FF)));
		}
	}

	return result;
}

String operator+(const String& left, const String& right)
{
	auto out(left);
	return out += right;
}

} // namespace re