#pragma once

#include <Core/String.hpp>
#include <Core/flat_map.hpp>
#include <IgniLang/BuildTarget.hpp>
#include <IgniLang/BuildType.hpp>

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
		re::String outputPath = "main";
		std::vector<re::String> sourceFiles;
		bool shouldRun = false;
		bool disableDCE = false;
	};

	static constexpr auto USAGE_HINT = "Usage: igni-cli [options] <file1.igni> <file2.igni> ...\n"
									   "Options:\n"
									   "  --dll      Build as dynamic library\n"
									   "  --exe      Build as executable\n"
									   "  -o         Set output file\n"
									   "  --run      Run program immediately after compilation"
									   "  --no-dce   Disable Dead Code Elimination\n";

	static constexpr auto NO_TARGET_ERROR = "[Error] No target provided.\n";
	static constexpr auto NO_SOURCE_ERROR = "[Error] No source files provided.\n";
	static constexpr auto NO_OUTPUT_ERROR = "[Error] No output path provided after '-o' option.\n";
	static constexpr auto NO_BUILD_TYPE_ERROR = "[Error] No build type provided.\n";

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

	re::String OutputPath() const
	{
		return m_options->outputPath;
	}

	bool ShouldRun() const
	{
		return m_options->shouldRun;
	}

	bool DisableDCE() const
	{
		return m_options->disableDCE;
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

		static constexpr auto TARGET_MAP = re::make_flat_map<re::HashedString, enum BuildTarget>({
			{ "--dotnet"_hs, BuildTarget::DotNet },
			{ "--rvm"_hs, BuildTarget::RVM },
		});

		static constexpr auto TYPE_MAP = re::make_flat_map<re::HashedString, enum BuildType>({
			{ "--dll"_hs, BuildType::DynamicLibrary },
			{ "--exe"_hs, BuildType::Executable },
		});

		for (int i = 1; i < argc; ++i)
		{
			const re::String arg = argv[i];
			const auto hashed = arg.Hashed();

			if (hashed == "--no-dce"_hs)
			{
				options.disableDCE = true;
				continue;
			}

			if (hashed == "-o"_hs)
			{
				if (argc < i + 1)
				{
					errors.emplace_back(NO_OUTPUT_ERROR);
					continue;
				}
				options.outputPath = argv[i + 1];
				i++;
				continue;
			}

			if (hashed == "--run"_hs)
			{
				options.shouldRun = true;
				continue;
			}

			if (const auto type = TYPE_MAP[hashed])
			{
				options.buildType = *type;
				continue;
			}

			if (const auto target = TARGET_MAP[hashed])
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