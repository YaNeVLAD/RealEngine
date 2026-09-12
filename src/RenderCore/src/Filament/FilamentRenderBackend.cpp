#include <RenderCore/Filament/FilamentRenderBackend.hpp>

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
#include <filament/Texture.h>
#include <filament/TextureSampler.h>
#include <filament/TransformManager.h>
#include <filament/VertexBuffer.h>
#include <filament/View.h>
#include <filament/Viewport.h>
#include <utils/Entity.h>
#include <utils/EntityManager.h>

#include <imgui.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <limits>
#include <ranges>
#include <unordered_map>

struct ImGuiIO;
namespace re::render
{

struct UIFrameData
{
	utils::Entity entity;
	filament::VertexBuffer* vb;
	filament::IndexBuffer* ib;
	std::vector<filament::MaterialInstance*> matInstances;
};

struct FilamentRenderBackend::Impl
{
	filament::Engine* engine = nullptr;
	filament::Scene* scene = nullptr;
	filament::View* view = nullptr;
	filament::Renderer* renderer = nullptr;
	filament::Camera* camera = nullptr;
	filament::SwapChain* swapChain = nullptr;
	utils::Entity cameraEntity;

	filament::Scene* uiScene = nullptr;
	filament::View* uiView = nullptr;
	filament::Camera* uiCamera = nullptr;
	utils::Entity uiCameraEntity;

	filament::Texture* fontTexture = nullptr;
	filament::Material* uiMaterial = nullptr;

	std::vector<UIFrameData> uiFrameData;

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

	m_impl->uiScene = m_impl->engine->createScene();
	m_impl->uiCameraEntity = utils::EntityManager::get().create();
	m_impl->uiCamera = m_impl->engine->createCamera(m_impl->uiCameraEntity);
	m_impl->uiView = m_impl->engine->createView();

	m_impl->uiView->setScene(m_impl->uiScene);
	m_impl->uiView->setCamera(m_impl->uiCamera);
	m_impl->uiView->setPostProcessingEnabled(false);

	m_impl->uiView->setBlendMode(filament::View::BlendMode::TRANSLUCENT);

	const ImGuiIO& io = ImGui::GetIO();
	unsigned char* pixels;
	int texWidth, texHeight;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &texWidth, &texHeight);

	m_impl->fontTexture = filament::Texture::Builder()
							  .width(static_cast<std::uint32_t>(texWidth))
							  .height(static_cast<std::uint32_t>(texHeight))
							  .levels(1)
							  .format(filament::Texture::InternalFormat::RGBA8)
							  .build(*m_impl->engine);

	filament::Texture::PixelBufferDescriptor pb(
		pixels, static_cast<std::size_t>(texWidth * texHeight * 4),
		filament::Texture::Format::RGBA, filament::Texture::Type::UBYTE);
	m_impl->fontTexture->setImage(*m_impl->engine, 0, std::move(pb));

	Resize(width, height);
	// LoadUIMaterial("imgui.filamat");
}

void FilamentRenderBackend::Shutdown()
{
	if (m_impl->engine)
	{
		// 1. ОЧИСТКА КАДРОВ И ИНСТАНСОВ (ДО БАЗОВОГО МАТЕРИАЛА)
		for (auto& fd : m_impl->uiFrameData)
		{
			m_impl->engine->destroy(fd.entity);
			m_impl->engine->destroy(fd.vb);
			m_impl->engine->destroy(fd.ib);
			for (auto* mi : fd.matInstances)
			{
				m_impl->engine->destroy(mi); // Удаляем инстанс до удаления базы
			}
			utils::EntityManager::get().destroy(fd.entity);
		}
		m_impl->uiFrameData.clear();

		// 2. ОЧИСТКА БАЗОВЫХ РЕСУРСОВ UI
		if (m_impl->uiMaterial)
			m_impl->engine->destroy(m_impl->uiMaterial);
		if (m_impl->fontTexture)
			m_impl->engine->destroy(m_impl->fontTexture);

		if (m_impl->uiView)
			m_impl->engine->destroy(m_impl->uiView);
		if (m_impl->uiScene)
			m_impl->engine->destroy(m_impl->uiScene);
		if (m_impl->uiCamera)
			m_impl->engine->destroyCameraComponent(m_impl->uiCameraEntity);
		utils::EntityManager::get().destroy(m_impl->uiCameraEntity);

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
		type = filament::LightManager::Type::POINT;
	else if (light.type == LightDataView::LightType::Spot)
		type = filament::LightManager::Type::SPOT;

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
}

// void FilamentRenderBackend::RenderUI(const UIDrawData& uiData)
// {
// 	const auto drawData = static_cast<ImDrawData*>(uiData.nativeData);
// 	if (!drawData || drawData->CmdListsCount == 0 || !m_impl->uiMaterial)
// 	{
// 		return;
// 	}
//
// 	// --- ИНИЦИАЛИЗАЦИЯ / ОБНОВЛЕНИЕ ШРИФТОВОГО АТЛАСА ---
// 	ImGuiIO& io = ImGui::GetIO();
// 	if (io.Fonts->IsBuilt() && !m_impl->fontTexture)
// 	{
// 		unsigned char* pixels;
// 		int texWidth, texHeight;
// 		io.Fonts->GetTexDataAsRGBA32(&pixels, &texWidth, &texHeight);
//
// 		m_impl->fontTexture = filament::Texture::Builder()
// 								  .width(static_cast<std::uint32_t>(texWidth))
// 								  .height(static_cast<std::uint32_t>(texHeight))
// 								  .levels(1)
// 								  .format(filament::Texture::InternalFormat::RGBA8)
// 								  .build(*m_impl->engine);
//
// 		filament::Texture::PixelBufferDescriptor pb(
// 			pixels, static_cast<std::size_t>(texWidth * texHeight * 4),
// 			filament::Texture::Format::RGBA, filament::Texture::Type::UBYTE);
//
// 		m_impl->fontTexture->setImage(*m_impl->engine, 0, std::move(pb));
//
// 		// Регистрируем ID текстуры в ImGui для обратной связи
// 		io.Fonts->SetTexID(reinterpret_cast<ImTextureID>(m_impl->fontTexture));
// 	}
//
// 	// Очистка предыдущего кадра UI
// 	for (auto& fd : m_impl->uiFrameData)
// 	{
// 		m_impl->uiScene->remove(fd.entity);
// 		m_impl->engine->destroy(fd.entity);
// 		m_impl->engine->destroy(fd.vb);
// 		m_impl->engine->destroy(fd.ib);
// 		for (auto* mi : fd.matInstances)
// 			m_impl->engine->destroy(mi);
// 		utils::EntityManager::get().destroy(fd.entity);
// 	}
// 	m_impl->uiFrameData.clear();
//
// 	const float L = drawData->DisplayPos.x;
// 	const float R = drawData->DisplayPos.x + drawData->DisplaySize.x;
// 	const float T = drawData->DisplayPos.y;
// 	const float B = drawData->DisplayPos.y + drawData->DisplaySize.y;
// 	m_impl->uiCamera->setProjection(filament::Camera::Projection::ORTHO, L, R, B, T, -1.0f, 1.0f);
//
// 	float fb_height = drawData->DisplaySize.y * drawData->FramebufferScale.y;
//
// 	for (int n = 0; n < drawData->CmdListsCount; n++)
// 	{
// 		const ImDrawList* cmd_list = drawData->CmdLists[n];
// 		UIFrameData frameData;
//
// 		frameData.vb = filament::VertexBuffer::Builder()
// 						   .vertexCount(cmd_list->VtxBuffer.Size)
// 						   .bufferCount(1)
// 						   .attribute(filament::VertexAttribute::POSITION, 0, filament::VertexBuffer::AttributeType::FLOAT2, offsetof(ImDrawVert, pos), sizeof(ImDrawVert))
// 						   .attribute(filament::VertexAttribute::UV0, 0, filament::VertexBuffer::AttributeType::FLOAT2, offsetof(ImDrawVert, uv), sizeof(ImDrawVert))
// 						   .attribute(filament::VertexAttribute::COLOR, 0, filament::VertexBuffer::AttributeType::UBYTE4, offsetof(ImDrawVert, col), sizeof(ImDrawVert))
// 						   .normalized(filament::VertexAttribute::COLOR)
// 						   .build(*m_impl->engine);
//
// 		void* vCopy = std::malloc(cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
// 		std::memcpy(vCopy, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
// 		frameData.vb->setBufferAt(*m_impl->engine, 0, filament::VertexBuffer::BufferDescriptor(vCopy, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert), [](void* b, size_t, void*) { std::free(b); }));
//
// 		// ПРЕВРАЩАЕМ В 32-БИТА: Жестко запекаем смещение VtxOffset во избежание "каши"
// 		frameData.ib = filament::IndexBuffer::Builder()
// 						   .indexCount(cmd_list->IdxBuffer.Size)
// 						   .bufferType(filament::IndexBuffer::IndexType::UINT)
// 						   .build(*m_impl->engine);
//
// 		uint32_t* iCopy = static_cast<uint32_t*>(std::malloc(cmd_list->IdxBuffer.Size * sizeof(uint32_t)));
// 		const ImDrawIdx* srcIdx = cmd_list->IdxBuffer.Data;
// 		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
// 		{
// 			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
// 			for (unsigned int i = 0; i < pcmd->ElemCount; i++)
// 			{
// 				iCopy[pcmd->IdxOffset + i] = static_cast<uint32_t>(srcIdx[pcmd->IdxOffset + i]) + pcmd->VtxOffset;
// 			}
// 		}
// 		frameData.ib->setBuffer(*m_impl->engine, filament::IndexBuffer::BufferDescriptor(iCopy, cmd_list->IdxBuffer.Size * sizeof(uint32_t), [](void* b, size_t, void*) { std::free(b); }));
//
// 		frameData.entity = utils::EntityManager::get().create();
// 		m_impl->engine->getTransformManager().create(frameData.entity);
//
// 		filament::RenderableManager::Builder builder(cmd_list->CmdBuffer.Size);
// 		builder.boundingBox(filament::Box{ { 0, 0, 0 }, { 10000, 10000, 10000 } }).culling(false);
//
// 		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
// 		{
// 			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
//
// 			// Создаем уникальный материал для каждой команды отрисовки (для Scissor)
// 			filament::MaterialInstance* mi = m_impl->uiMaterial->createInstance();
// 			filament::TextureSampler sampler(filament::TextureSampler::MinFilter::LINEAR, filament::TextureSampler::MagFilter::LINEAR);
// 			mi->setParameter("albedo", m_impl->fontTexture, sampler);
//
// 			// Конвертируем ClipRect ImGui в координаты Filament (с инверсией Y)
// 			float clipMinX = pcmd->ClipRect.x * drawData->FramebufferScale.x;
// 			float clipMinY = pcmd->ClipRect.y * drawData->FramebufferScale.y;
// 			float clipMaxX = pcmd->ClipRect.z * drawData->FramebufferScale.x;
// 			float clipMaxY = pcmd->ClipRect.w * drawData->FramebufferScale.y;
//
// 			float filClipMinY = fb_height - clipMaxY;
// 			float filClipMaxY = fb_height - clipMinY;
//
// 			mi->setParameter("clipRect", filament::math::float4{ clipMinX, filClipMinY, clipMaxX, filClipMaxY });
//
// 			builder.geometry(cmd_i, filament::RenderableManager::PrimitiveType::TRIANGLES, frameData.vb, frameData.ib, pcmd->IdxOffset, pcmd->ElemCount);
// 			builder.material(cmd_i, mi);
// 			builder.blendOrder(cmd_i, cmd_i);
//
// 			frameData.matInstances.push_back(mi);
// 		}
// 		builder.build(*m_impl->engine, frameData.entity);
//
// 		m_impl->uiScene->addEntity(frameData.entity);
// 		m_impl->uiFrameData.push_back(std::move(frameData));
// 	}
// }

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