#pragma once

#include <Core/Export.hpp>

#include <compare>
#include <string>

namespace re
{

class RE_CORE_API String
{
public:
	using Iterator = std::u32string::iterator;
	using ConstIterator = std::u32string::const_iterator;

	static constexpr std::size_t NPos = std::u32string::npos;

	String() = default;

	String(std::nullptr_t) = delete;

	String(const String&) = default;
	String(String&&) = default;

	String& operator=(const String&) = default;
	String& operator=(String&&) = default;

	String(const char32_t* u32Str);
	String(std::u32string u32Str);
	String(char32_t ch);

	String(const char* u8Str);
	String(std::string_view u8Str);
	String(std::string const& u8Str);
	String(char ch);

	String(const wchar_t* wStr);
	String(std::wstring_view wStr);
	String(std::wstring const& wStr);
	String(wchar_t ch);

	operator std::string() const;

	operator std::wstring() const;

	[[nodiscard]] std::string ToString() const;

	[[nodiscard]] std::wstring ToWString() const;

	[[nodiscard]] std::u32string ToU32String() const;

	[[nodiscard]] const char32_t* Data() const;

	[[nodiscard]] Iterator begin();
	[[nodiscard]] ConstIterator begin() const;

	[[nodiscard]] Iterator end();
	[[nodiscard]] ConstIterator end() const;

	[[nodiscard]] std::size_t Length() const;
	[[nodiscard]] std::size_t Size() const;
	[[nodiscard]] bool Empty() const;

	void Reserve(std::size_t newCap);

	void Clear();

	void Resize(std::size_t count);

	char32_t& operator[](std::size_t pos);
	const char32_t& operator[](std::size_t pos) const;

	void Append(const String& other);

	void PushBack(char32_t ch);

	[[nodiscard]] std::size_t Find(const String& str, std::size_t pos = 0) const;

	[[nodiscard]] std::size_t Find(const char32_t& ch, std::size_t pos = 0) const;

	[[nodiscard]] String Substring(std::size_t pos = 0, std::size_t count = NPos) const;

	static char32_t ToUpper(char32_t cp);
	static char32_t ToLower(char32_t cp);

	[[nodiscard]] String ToUpper() const;
	[[nodiscard]] String ToLower() const;

	String& operator+=(const String& other);

	auto operator<=>(const String&) const = default;

private:
	static std::u32string DecodeUtf8(std::string_view u8Str);
	static std::string EncodeUtf8(std::u32string const& u32Str);

	static std::u32string DecodeWide(std::wstring_view wStr);
	static std::wstring EncodeWide(std::u32string const& u32Str);

private:
	std::u32string m_string;
};

[[nodiscard]] RE_CORE_API String operator+(const String& left, const String& right);

} // namespace re