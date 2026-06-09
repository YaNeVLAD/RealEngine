#include "TestUtils.hpp"

#include <IgniLang/BuildTarget.hpp>
#include <IgniLang/Compiler/DotNetBackend.hpp>
#include <IgniLang/Compiler/Pipeline.hpp>

#include <filesystem>
#include <fstream>

#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#endif

namespace fs = std::filesystem;

class DotNetE2ETestFixture : public ::testing::TestWithParam<fs::path>
{
protected:
	static std::string s_ilasmPath;

	static void SetUpTestSuite()
	{
		s_ilasmPath = FindILAsm();
	}

	static std::string FindILAsm()
	{
#ifdef _WIN32
		std::vector<std::string> paths = {
			"C:\\Windows\\Microsoft.NET\\Framework64\\v4.0.30319\\ilasm.exe",
			"C:\\Windows\\Microsoft.NET\\Framework\\v4.0.30319\\ilasm.exe",
			"ilasm"
		};
		for (const auto& path : paths)
		{
			if (path == "ilasm")
			{
				int exitCode = 0;
				RunCommand("ilasm /?", exitCode);
				if (exitCode >= 0)
				{
					return path;
				}
			}
			else if (fs::exists(path))
			{
				return "\"" + path + "\"";
			}
		}
#else
		int exitCode = 0;
		RunCommand("ilasm --version", exitCode);
		if (exitCode >= 0)
		{
			return "ilasm";
		}
#endif
		return "";
	}

	static std::string RunCommand(const std::string& cmd, int& outExitCode)
	{
		std::string result;
		char buffer[128];
		FILE* pipe = popen(cmd.c_str(), "r");
		if (!pipe)
		{
			outExitCode = -1;
			return "";
		}
		while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
		{
			result += buffer;
		}
		outExitCode = pclose(pipe);
		return result;
	}

	static std::string GetRunCommand(const std::string& exePath)
	{
#ifdef _WIN32
		return exePath;
#else
		return "mono " + exePath;
#endif
	}
};

std::string DotNetE2ETestFixture::s_ilasmPath = "";

TEST_P(DotNetE2ETestFixture, ExecutesCorrectlyOnDotNet)
{
	if (s_ilasmPath.empty())
	{
		GTEST_SKIP() << "ilasm assembler not found in the system. Skipping .NET E2E tests.";
	}

#ifndef _WIN32
	int monoCheckCode = 0;
	RunCommand("mono --version", monoCheckCode);
	if (monoCheckCode < 0)
	{
		GTEST_SKIP() << "mono runtime not found. Skipping .NET E2E tests.";
	}
#endif

	const fs::path& scriptPath = GetParam();
	std::string expectedOutput = ExtractExpectedOutput(scriptPath);

	std::vector<re::String> sourceFiles;
	if (fs::exists("assets/source/stdlib_dotnet.igni"))
	{
		sourceFiles.emplace_back("assets/source/stdlib_dotnet.igni");
	}
	sourceFiles.emplace_back(scriptPath.string());

	igni::compiler::Pipeline pipeline("assets/igni_grammar.txt");
	igni::compiler::DotNetBackend backend;

	std::string cilCode;
	ASSERT_NO_THROW({
		auto result = pipeline.Compile(sourceFiles, igni::BuildTarget::DotNet, igni::BuildType::Executable, false, backend);
		ASSERT_TRUE(result.success) << "Compilation failed for script: " << scriptPath.filename().string();
		cilCode = result.generatedCode;
	}) << "Compiler pipeline crashed on: "
	   << scriptPath.filename().string();

	ASSERT_FALSE(cilCode.empty()) << "Compiler returned empty CIL code";

	std::string tempIlName = "temp_test_" + scriptPath.stem().string() + ".il";
	std::string tempExeName = "temp_test_" + scriptPath.stem().string() + ".exe";

	std::ofstream out(tempIlName);
	out << cilCode;
	out.close();

	std::cout << cilCode;

	int compileExitCode = 0;
	std::string compileCmd = s_ilasmPath + " /quiet /exe /output=" + tempExeName + " " + tempIlName;
#ifndef _WIN32
	compileCmd = s_ilasmPath + " /exe /output=" + tempExeName + " " + tempIlName + " > /dev/null 2>&1";
#endif

	RunCommand(compileCmd, compileExitCode);
	ASSERT_EQ(compileExitCode, 0) << "ilasm failed to assemble generated CIL for " << scriptPath.filename().string();

	int runExitCode = 0;
	std::string runCmd = GetRunCommand(tempExeName);
	std::string capturedOutput = RunCommand(runCmd, runExitCode);

	fs::remove(tempIlName);
	fs::remove(tempExeName);
#ifdef _WIN32
	fs::remove("temp_test_" + scriptPath.stem().string() + ".pdb");
#endif

	ASSERT_EQ(runExitCode, 0) << ".NET Execution failed or crashed";
	EXPECT_EQ(capturedOutput, expectedOutput) << "Output mismatch on .NET for script: " << scriptPath.filename().string();
}

inline std::vector<fs::path> GetDotNetCompatibleScripts()
{
	const std::vector<fs::path> allScripts = GetTestScripts();
	std::vector<fs::path> compatible;
	for (const auto& path : allScripts)
	{
		if (std::string name = path.stem().string();
			name == "01_primitives_math_dotnet"
			|| name == "02_control_flow"
			|| name == "03_functions"
			|| name == "04_classes_oop"
			|| name == "05_generics"
			|| name == "06_deep_closures"
			|| name == "07_complex_generics"
			|| name == "08_virtual_dispatch"
			|| name == "09_varargs_packing_dotnet"
			|| name == "15_type_casting_dotnet"
			|| name == "math")
		{
			compatible.push_back(path);
		}
	}
	return compatible;
}

INSTANTIATE_TEST_SUITE_P(
	IgniDotNetScripts,
	DotNetE2ETestFixture,
	::testing::ValuesIn(GetDotNetCompatibleScripts()),
	[](const ::testing::TestParamInfo<fs::path>& info) {
		std::string name = info.param.stem().string();
		std::replace_if(
			name.begin(), name.end(), [](char c) { return !std::isalnum(c); }, '_');
		return name;
	});