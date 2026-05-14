#pragma once

#include <string>

namespace igni::cli
{

class IRunner
{
public:
	virtual ~IRunner() = default;

	virtual int Run(const std::string& generatedCode) = 0;
};

} // namespace igni::cli