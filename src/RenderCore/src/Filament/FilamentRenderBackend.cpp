#include <RenderCore/Filament/FilamentRenderBackend.hpp>

#include <filagui/ImGuiHelper.h>

#include <filament/Box.h>
#include <filament/Camera.h>
#include <filament/Engine.h>
#include <filament/IndexBuffer.h>
#include <filament/LightManager.h>
#include <filament/Material.h>
#include <filament/RenderableManager.h>
#include <filament/Renderer.h>
#include <filament/Scene.h>
#include <filament/SwapChain.h>
#include <filament/TransformManager.h>
#include <filament/VertexBuffer.h>
#include <filament/View.h>
#include <filament/Viewport.h>
#include <utils/Entity.h>
#include <utils/EntityManager.h>

#include <imgui.h>

#include <algorithm>
#include <limits>
#include <memory>
#include <ranges>
#include <unordered_map>

namespace re::render
{

struct FilamentRenderBackend::Impl
{
	filament::Engine* engine = nullptr;
	filament::Scene* scene = nullptr;
	filament::View* view = nullptr;
	filament::Renderer* renderer = nullptr;
	filament::Camera* camera = nullptr;
	filament::SwapChain* swapChain = nullptr;
	utils::Entity cameraEntity;

	filament::View* uiView = nullptr;

	std::unique_ptr<filagui::ImGuiHelper> uiHelper;

	std::uint64_t nextId = 0;
	std::unordered_map<std::uint64_t, utils::Entity> entityMap;

	struct RenderResources
	{
		filament::VertexBuffer* vb = nullptr;
		filament::IndexBuffer* ib = nullptr;
	};
	std::unordered_map<std::uint64_t, RenderResources> resourcesMap;
};

FilamentRenderBackend::FilamentRenderBackend()
	: m_impl(std::make_unique<Impl>())
{
}

FilamentRenderBackend::~FilamentRenderBackend()
{
	Shutdown();
}

void FilamentRenderBackend::Init(void* windowHandle, const std::uint32_t width, const std::uint32_t height)
{
	m_impl->engine = filament::Engine::create();
	m_impl->scene = m_impl->engine->createScene();
	m_impl->view = m_impl->engine->createView();
	m_impl->renderer = m_impl->engine->createRenderer();
	m_impl->swapChain = m_impl->engine->createSwapChain(windowHandle);

	m_impl->view->setPostProcessingEnabled(false);

	m_impl->cameraEntity = utils::EntityManager::get().create();
	m_impl->camera = m_impl->engine->createCamera(m_impl->cameraEntity);

	m_impl->view->setScene(m_impl->scene);
	m_impl->view->setCamera(m_impl->camera);

	// UI overlay view: filagui fills it with its own scene, camera and renderable.
	m_impl->uiView = m_impl->engine->createView();
	m_impl->uiView->setPostProcessingEnabled(false);
	m_impl->uiView->setBlendMode(filament::View::BlendMode::TRANSLUCENT);

	// filagui creates its own ImGui scene/camera/material and shares the ImGui context
	// that gui::Context created at startup (so input state stays in one context).
	m_impl->uiHelper = std::make_unique<filagui::ImGuiHelper>(
		m_impl->engine,
		m_impl->uiView,
		utils::Path("assets/Roboto.ttf"),
		ImGui::GetCurrentContext());

	Resize(width, height);
}

void FilamentRenderBackend::Shutdown()
{
	if (m_impl->engine)
	{
		m_impl->uiHelper.reset();
		for (auto& [vb, ib] : m_impl->resourcesMap | std::views::values)
		{
			if (vb)
			{
				m_impl->engine->destroy(vb);
			}
			if (ib)
			{
				m_impl->engine->destroy(ib);
			}
		}
		m_impl->resourcesMap.clear();

		for (const auto& entity : m_impl->entityMap | std::views::values)
		{
			m_impl->engine->destroy(entity);
		}
		m_impl->entityMap.clear();

		if (m_impl->uiView)
		{
			m_impl->engine->destroy(m_impl->uiView);
		}
		if (m_impl->swapChain)
		{
			m_impl->engine->destroy(m_impl->swapChain);
		}
		if (m_impl->view)
		{
			m_impl->engine->destroy(m_impl->view);
		}
		if (m_impl->scene)
		{
			m_impl->engine->destroy(m_impl->scene);
		}
		if (m_impl->renderer)
		{
			m_impl->engine->destroy(m_impl->renderer);
		}
		if (m_impl->camera)
		{
			m_impl->engine->destroyCameraComponent(m_impl->cameraEntity);
		}

		utils::EntityManager::get().destroy(m_impl->cameraEntity);
		filament::Engine::destroy(&m_impl->engine);
	}
}

void FilamentRenderBackend::Resize(std::uint32_t width, std::uint32_t height)
{
	if (m_impl->view && width > 0 && height > 0)
	{
		m_impl->view->setViewport({ 0, 0, width, height });
		m_impl->uiView->setViewport({ 0, 0, width, height });
	}
	if (m_impl->uiHelper)
	{
		m_impl->uiHelper->setDisplaySize(width, height);
	}
}

RenderEntityHandle FilamentRenderBackend::CreateStaticMesh(const MeshDataView& mesh, const MaterialDataView& material)
{
	if (mesh.vertexCount == 0 || !mesh.vertices || mesh.indexCount == 0 || !mesh.indices)
	{
		return RenderEntityHandle{ 0 };
	}

	utils::Entity nativeEntity = utils::EntityManager::get().create();
	std::uint64_t id = ++m_impl->nextId;

	filament::VertexBuffer* vb = filament::VertexBuffer::Builder()
									 .vertexCount(mesh.vertexCount)
									 .bufferCount(1)
									 .attribute(filament::VertexAttribute::POSITION, 0, filament::VertexBuffer::AttributeType::FLOAT3, 0, mesh.vertexStride)
									 .attribute(filament::VertexAttribute::COLOR, 0, filament::VertexBuffer::AttributeType::UBYTE4, 24, mesh.vertexStride)
									 .normalized(filament::VertexAttribute::COLOR)
									 .attribute(filament::VertexAttribute::UV0, 0, filament::VertexBuffer::AttributeType::FLOAT2, 28, mesh.vertexStride)
									 .attribute(filament::VertexAttribute::TANGENTS, 0, filament::VertexBuffer::AttributeType::FLOAT4, 72, mesh.vertexStride)
									 .build(*m_impl->engine);

	const size_t vSize = mesh.vertexCount * mesh.vertexStride;
	void* vCopy = std::malloc(vSize);
	std::memcpy(vCopy, mesh.vertices, vSize);
	vb->setBufferAt(*m_impl->engine, 0, filament::VertexBuffer::BufferDescriptor(vCopy, vSize, [](void* buffer, size_t, void*) { std::free(buffer); }));

	filament::IndexBuffer* ib = filament::IndexBuffer::Builder()
									.indexCount(mesh.indexCount)
									.bufferType(mesh.is32BitIndices ? filament::IndexBuffer::IndexType::UINT : filament::IndexBuffer::IndexType::USHORT)
									.build(*m_impl->engine);

	const size_t iSize = mesh.indexCount * (mesh.is32BitIndices ? 4 : 2);
	void* iCopy = std::malloc(iSize);
	std::memcpy(iCopy, mesh.indices, iSize);
	ib->setBuffer(*m_impl->engine, filament::IndexBuffer::BufferDescriptor(iCopy, iSize, [](void* buffer, size_t, void*) { std::free(buffer); }));

	filament::math::float3 minBound{ std::numeric_limits<float>::max() };
	filament::math::float3 maxBound{ std::numeric_limits<float>::lowest() };

	const auto* vData = static_cast<const float*>(mesh.vertices);
	const size_t strideFloats = mesh.vertexStride / sizeof(float);

	for (size_t i = 0; i < mesh.vertexCount; ++i)
	{
		float x = vData[i * strideFloats + 0];
		float y = vData[i * strideFloats + 1];
		float z = vData[i * strideFloats + 2];

		if (std::isnan(x) || std::isnan(y) || std::isnan(z))
		{
			continue;
		}

		minBound.x = std::min(minBound.x, x);
		minBound.y = std::min(minBound.y, y);
		minBound.z = std::min(minBound.z, z);
		maxBound.x = std::max(maxBound.x, x);
		maxBound.y = std::max(maxBound.y, y);
		maxBound.z = std::max(maxBound.z, z);
	}

	if (minBound.x > maxBound.x)
	{
		minBound = { -0.1f, -0.1f, -0.1f };
		maxBound = { 0.1f, 0.1f, 0.1f };
	}

	filament::math::float3 extent = (maxBound - minBound) * 0.5f;
	extent.x = std::max(extent.x, 0.001f);
	extent.y = std::max(extent.y, 0.001f);
	extent.z = std::max(extent.z, 0.001f);

	filament::RenderableManager::Builder(1)
		.boundingBox(filament::Box{ (maxBound + minBound) * 0.5f, extent })
		.geometry(0, filament::RenderableManager::PrimitiveType::TRIANGLES, vb, ib)
		.culling(true)
		.build(*m_impl->engine, nativeEntity);

	m_impl->scene->addEntity(nativeEntity);
	m_impl->entityMap[id] = nativeEntity;
	m_impl->resourcesMap[id] = Impl::RenderResources{ vb, ib };

	return RenderEntityHandle{ id };
}

RenderEntityHandle FilamentRenderBackend::CreateLight(const LightDataView& light)
{
	const utils::Entity lightEntity = utils::EntityManager::get().create();
	const std::uint64_t id = ++m_impl->nextId;

	auto type = filament::LightManager::Type::DIRECTIONAL;
	if (light.type == LightDataView::LightType::Light)
	{
		type = filament::LightManager::Type::POINT;
	}
	else if (light.type == LightDataView::LightType::Spot)
	{
		type = filament::LightManager::Type::SPOT;
	}

	filament::LightManager::Builder(type)
		.color(filament::Color::toLinear<filament::ACCURATE>({ light.color.r, light.color.g, light.color.b }))
		.intensity(light.intensity * 100000.0f)
		.position({ light.position.x, light.position.y, light.position.z })
		.direction({ light.direction.x, light.direction.y, light.direction.z })
		.falloff(50.0f)
		.build(*m_impl->engine, lightEntity);

	m_impl->scene->addEntity(lightEntity);
	m_impl->entityMap[id] = lightEntity;

	return RenderEntityHandle{ id };
}

void FilamentRenderBackend::DestroyEntity(const RenderEntityHandle handle)
{
	if (const auto it = m_impl->entityMap.find(handle.id); it != m_impl->entityMap.end())
	{
		m_impl->scene->remove(it->second);
		m_impl->engine->destroy(it->second);

		if (const auto resIt = m_impl->resourcesMap.find(handle.id); resIt != m_impl->resourcesMap.end())
		{
			if (resIt->second.vb)
				m_impl->engine->destroy(resIt->second.vb);
			if (resIt->second.ib)
				m_impl->engine->destroy(resIt->second.ib);
			m_impl->resourcesMap.erase(resIt);
		}

		utils::EntityManager::get().destroy(it->second);
		m_impl->entityMap.erase(it);
	}
}

void FilamentRenderBackend::UpdateTransform(const RenderEntityHandle handle, const glm::mat4& transformMatrix)
{
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			if (std::isnan(transformMatrix[i][j]) || std::isinf(transformMatrix[i][j]))
			{
				return;
			}
		}
	}

	if (const auto it = m_impl->entityMap.find(handle.id); it != m_impl->entityMap.end())
	{
		auto& tcm = m_impl->engine->getTransformManager();
		const auto instance = tcm.getInstance(it->second);
		if (instance)
		{
			tcm.setTransform(instance, *reinterpret_cast<const filament::math::mat4f*>(&transformMatrix));
		}
	}
}

void FilamentRenderBackend::UpdateLight(const RenderEntityHandle handle, const LightDataView& light)
{
	if (const auto it = m_impl->entityMap.find(handle.id); it != m_impl->entityMap.end())
	{
		auto& lm = m_impl->engine->getLightManager();
		const auto instance = lm.getInstance(it->second);
		if (instance)
		{
			lm.setPosition(instance, { light.position.x, light.position.y, light.position.z });
			lm.setDirection(instance, { light.direction.x, light.direction.y, light.direction.z });
			lm.setColor(instance, filament::Color::toLinear<filament::ACCURATE>({ light.color.r, light.color.g, light.color.b }));
			lm.setIntensity(instance, light.intensity * 100000.0f);
		}
	}
}

void FilamentRenderBackend::UpdateCamera(const CameraDataView& camera)
{
	if (std::isnan(camera.fov) || std::isnan(camera.aspect))
	{
		return;
	}

	const glm::mat4 inverseView = glm::inverse(camera.viewMatrix);

	m_impl->camera->setProjection(camera.fov, camera.aspect, camera.nearClip, camera.farClip, filament::Camera::Fov::VERTICAL);
	m_impl->camera->setExposure(16.0f, 1.0f / 125.0f, 100.0f);
	m_impl->camera->setModelMatrix(*reinterpret_cast<const filament::math::mat4f*>(&inverseView));
}

void FilamentRenderBackend::SetSkybox(std::uint32_t cubemapID, std::uint32_t irradianceID) {}

void FilamentRenderBackend::SetClearColor(const Color color)
{
	if (m_impl->renderer)
	{
		const auto& c = color.ToFloat();
		filament::Renderer::ClearOptions options;
		options.clearColor = { c.r, c.g, c.b, c.a };
		options.clear = true;
		options.discard = true;
		m_impl->renderer->setClearOptions(options);
	}
}

void FilamentRenderBackend::RenderUI(float dt, std::function<void()> uiCallback)
{
	if (m_impl->uiHelper)
	{
		m_impl->uiHelper->render(dt, [uiCallback](filament::Engine*, filament::View*) {
			uiCallback();
		});
	}
}

void FilamentRenderBackend::BeginFrame() {}

void FilamentRenderBackend::RenderFrame()
{
	if (m_impl->renderer && m_impl->swapChain)
	{
		if (m_impl->renderer->beginFrame(m_impl->swapChain))
		{
			m_impl->renderer->render(m_impl->view);
			m_impl->renderer->render(m_impl->uiView);

			m_impl->renderer->endFrame();
		}
	}
}

void FilamentRenderBackend::EndFrame() {}

} // namespace re::render