#include <Scripting/CSharp/DotNetScriptEngine.hpp>

#include <filesystem>

namespace
{

std::string ArgOr(const int argc, char** argv, const int index, const char* fallback)
{
	return argc > index && argv[index][0] != '\0' ? argv[index] : fallback;
}

} // namespace

#define LOG(...) RE_LOG_INFO("Bootstrap"_logcat, __VA_ARGS__)
#define ERR(...) RE_LOG_INFO("Bootstrap"_logcat, __VA_ARGS__)

//   Env RE_EDITOR_AUTOCLOSE_MS — see RealEngineEditor
int main(int argc, char** argv)
{
	using namespace re::literals;
	const auto assemblyAbs = std::filesystem::absolute(ArgOr(argc, argv, 1, "RealEngineEditor.dll"));
	const auto typeName = re::String(ArgOr(argc, argv, 2, "RealEngineEditor.EditorBootstrap, RealEngineEditor"));
	const auto methodName = re::String(ArgOr(argc, argv, 3, "RunEditor"));

	std::filesystem::path configAbs = assemblyAbs;
	configAbs.replace_extension(".runtimeconfig.json");
	if (!std::filesystem::exists(configAbs))
	{
		configAbs = std::filesystem::absolute("EngineAPI.runtimeconfig.json");
	}

	try
	{
		re::scripting::EngineApiPointers apiPointers{};
		apiPointers.apiVersion = re::scripting::EngineApiVersion;

		re::DotNetScriptEngine engine;
		engine.Init(apiPointers, re::String(configAbs.string()));
		LOG(".NET host initialized");

		const auto assemblyPath = re::String(assemblyAbs.string());
		if (!engine.LoadAssembly(assemblyPath))
		{
			ERR("Failed to load assembly");
			return 2;
		}
		LOG("Assembly loaded");

		using RunEditorFn = int (*)();
		const auto runEditor = reinterpret_cast<RunEditorFn>(
			engine.LoadManagedEntryPoint(assemblyPath, typeName, methodName));

		LOG("Entering managed editor...");

		const int exitCode = runEditor();
		LOG("Editor exited with code {}", exitCode);

		return exitCode;
	}
	catch (const std::exception& ex)
	{
		ERR("Fatal: {}", ex.what());
		return 1;
	}
}
