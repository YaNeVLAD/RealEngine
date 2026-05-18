#pragma once

#include <Core/FlatMap.hpp>
#include <Core/String.hpp>
#include <IgniCLI/BuildType.hpp>
#include <IgniLang/BuildTarget.hpp>

#include <iostream>
#include <optional>
#include <vector>

namespace igni::cli
{

class CLArguments final
{
	struct Options
	{
		BuildTarget buildTarget = BuildTarget::Unknown;
		BuildType buildType = BuildType::Unknown;
		std::vector<re::String> sourceFiles;
	};

	static constexpr auto USAGE_HINT = "Usage: igni-cli [options] <file1.igni> <file2.igni> ...\n"
									   "Options:\n"
									   "  --rvm      Compile for RVM\n"
									   "  --dotnet   Compile for .NET CIL\n"
									   "  --dll      Build as dynamic library\n"
									   "  --exe      Build as executable\n";

	static constexpr auto NO_TARGET_ERROR = "[Error] No target provided.\n";
	static constexpr auto NO_SOURCE_ERROR = "[Error] No source files provided.\n";
	static constexpr auto NO_BUILD_TYPE_ERROR = "[Error] No build type provided.\n";

	template <typename TEnum, std::size_t N>
	using EnumMap = re::FlatMap<re::HashedString, TEnum, N>;

	template <std::size_t N>
	using BuildTypeMap = EnumMap<BuildType, N>;

	template <std::size_t N>
	using BuildTargetMap = EnumMap<BuildTarget, N>;

public:
	CLArguments(const int argc, char** argv)
	{
		m_options = Parse(argc, argv, m_errors);
	}

	bool Valid() const
	{
		return m_options.has_value();
	}

	BuildTarget BuildTarget() const
	{
		return m_options->buildTarget;
	}

	BuildType BuildType() const
	{
		return m_options->buildType;
	}

	std::vector<re::String>& SourceFiles()
	{
		return m_options->sourceFiles;
	}

	std::vector<re::String> const& SourceFiles() const
	{
		return m_options->sourceFiles;
	}

	std::vector<re::String> const& Errors() const
	{
		return m_errors;
	}

private:
	static std::optional<Options> Parse(const int argc, char** argv, std::vector<re::String>& errors)
	{
		using namespace re::literals;

		if (argc < 2)
		{
			errors.emplace_back(USAGE_HINT);
		}

		Options options;

		static constexpr BuildTargetMap TARGET_MAP = { {
			{ "--dotnet"_hs, BuildTarget::DotNet },
			{ "--rvm"_hs, BuildTarget::RVM },
		} };

		static constexpr BuildTypeMap TYPE_MAP = { {
			{ "--dll"_hs, BuildType::DynamicLibrary },
			{ "--exe"_hs, BuildType::Executable },
		} };

		for (int i = 1; i < argc; ++i)
		{
			const re::String arg = argv[i];
			const auto hashed = arg.Hashed();

			const auto type = TYPE_MAP[hashed];
			if (type)
			{
				options.buildType = *type;
				continue;
			}

			const auto target = TARGET_MAP[hashed];
			if (target)
			{
				options.buildTarget = *target;
				continue;
			}

			if (!arg.Empty() && arg[0] == '-')
			{
				errors.emplace_back("[Warning] Unknown flag ignored: " + arg + "\n");
			}
			else
			{
				options.sourceFiles.emplace_back(arg);
			}
		}

		if (options.sourceFiles.empty())
		{
			errors.emplace_back(NO_SOURCE_ERROR);
		}

		if (options.buildTarget == BuildTarget::Unknown)
		{
			errors.emplace_back(NO_TARGET_ERROR);
		}

		if (options.buildType == BuildType::Unknown)
		{
			errors.emplace_back(NO_BUILD_TYPE_ERROR);
		}

		if (!errors.empty())
		{
			return std::nullopt;
		}

		return options;
	}

private:
	std::optional<Options> m_options = std::nullopt;

	std::vector<re::String> m_errors{};
};

} // namespace igni::cli