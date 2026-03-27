#pragma once

#include <miniaudio.h>

#include <string>
#include <system_error>

namespace re::audio
{

inline const std::error_category& ErrorCategory()
{
	class AudioErrorCategory : public std::error_category
	{
	public:
		const char* name() const noexcept override
		{
			return "Audio error category";
		}

		std::string message(int errorCode) const override
		{
			return ma_result_description(static_cast<ma_result>(errorCode));
		}
	};

	static AudioErrorCategory errorCategory;
	return errorCategory;
}

} // namespace re::audio