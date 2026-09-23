#include <Runtime/Internal/ScriptBinder.hpp>
#include <Scripting/CSharp/DotNetScriptEngine.hpp>

#include "EditorViewport.hpp"

#include <filesystem>

namespace
{

std::string ArgOr(const int argc, char** argv, const int index, const char* fallback)
{
	return argc > index && argv[index][0] != '\0' ? argv[index] : fallback;
}

void SeedScene(re::ecs::Scene& scene, const re::scripting::EngineApiPointers& api)
{
	scene.CreateEntity()
		.Add<re::Dirty<re::TransformComponent>>()
		.Add<re::TransformComponent>({
			.position = { 0.f, 1.5f, 3.f },
			.rotation = { -26.6f, 0.f, 0.f },
		})
		.Add<re::CameraComponent>()
		.Add<re::NameComponent>("MainCamera");

	scene.CreateEntity()
		.Add<re::Dirty<re::TransformComponent>>()
		.Add<re::TransformComponent>({ .position = { 0.f, 2.f, 1.f } })
		.Add<re::LightComponent>(re::LightComponent::CreateDirectional(re::Color::White))
		.Add<re::NameComponent>("Sun");

	const std::uint64_t cube = api.Scene_SpawnPrimitive(
		static_cast<std::int32_t>(re::scripting::PrimitiveKind::Cube), re::Color::Red.ToInt());

	if (cube != re::ecs::Entity::INVALID_ID.Id() && api.Entity_SetName)
	{
		api.Entity_SetName(cube, "SeedCube");
	}
}

std::unique_ptr<EditorViewport> g_viewport;

bool Viewport_Create(const std::uint64_t hwnd, const std::uint32_t width, const std::uint32_t height)
{
	if (g_viewport || hwnd == 0 || width == 0 || height == 0)
	{
		return false;
	}

	g_viewport = std::make_unique<EditorViewport>();
	if (!g_viewport->Create(hwnd, width, height))
	{
		g_viewport.reset();
		return false;
	}

	SeedScene(g_viewport->Scene(), re::runtime::ScriptBinder::CreateApiPointers());
	return true;
}

bool Viewport_Render()
{
	return g_viewport && g_viewport->Render();
}

void Viewport_Resize(const std::uint32_t width, const std::uint32_t height)
{
	if (g_viewport)
	{
		g_viewport->Resize(width, height);
	}
}

void Viewport_Destroy()
{
	g_viewport.reset();
}

void BindViewportApi(re::scripting::EngineApiPointers& api)
{
	api.Viewport_Create = &Viewport_Create;
	api.Viewport_Render = &Viewport_Render;
	api.Viewport_Resize = &Viewport_Resize;
	api.Viewport_Destroy = &Viewport_Destroy;
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
		auto apiPointers = re::runtime::ScriptBinder::CreateApiPointers();
		BindViewportApi(apiPointers);

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

		using RunEditorFn = int (*)(void*);
		const auto runEditor = reinterpret_cast<RunEditorFn>(
			engine.LoadManagedEntryPoint(assemblyPath, typeName, methodName));

		LOG("Entering managed editor...");

		const int exitCode = runEditor(const_cast<re::scripting::EngineApiPointers*>(&apiPointers));
		LOG("Editor exited with code {}", exitCode);

		return exitCode;
	}
	catch (const std::exception& ex)
	{
		ERR("Fatal: {}", ex.what());
		return 1;
	}
}
