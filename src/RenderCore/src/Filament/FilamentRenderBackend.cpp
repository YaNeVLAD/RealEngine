#include <RenderCore/Filament/FilamentRenderBackend.hpp>

#include <RenderCore/Filament/Texture.hpp>

#include <filagui/ImGuiHelper.h>

#include <filament/Box.h>
#include <filament/Camera.h>
#include <filament/Engine.h>
#include <filament/IndexBuffer.h>
#include <filament/IndirectLight.h>
#include <filament/LightManager.h>
#include <filament/Material.h>
#include <filament/MaterialInstance.h>
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

#include "lit_resources.h"

#include <algorithm>
#include <array>
#include <limits>
#include <memory>
#include <ranges>
#include <unordered_map>

namespace re::render
{

namespace
{

filament::Texture* CreateSolidTexture(filament::Engine* engine, const std::uint8_t r, const std::uint8_t g, const std::uint8_t b, const std::uint8_t a)
{
	const std::array pixel{ r, g, b, a };
	auto* texture = filament::Texture::Builder()
						.width(1)
						.height(1)
						.levels(1)
						.format(filament::Texture::InternalFormat::RGBA8)
						.sampler(filament::Texture::Sampler::SAMPLER_2D)
						.build(*engine);

	texture->setImage(*engine, 0, filament::Texture::PixelBufferDescriptor(pixel.data(), pixel.size(), filament::Texture::Format::RGBA, filament::Texture::Type::UBYTE));
	return texture;
}

filament::Texture* GetOrCreateTexture(filament::Engine* engine, std::unordered_map<const Texture*, filament::Texture*>& cache, const Texture* texture, filament::Texture* fallback)
{
	if (!texture)
	{
		return fallback;
	}

	if (const auto cached = cache.find(texture); cached != cache.end())
	{
		return cached->second;
	}

	const auto& pixels = texture->GetPixelData();
	if (pixels.empty() || texture->Width() == 0 || texture->Height() == 0)
	{
		return fallback;
	}

	auto* created = filament::Texture::Builder()
						.width(texture->Width())
						.height(texture->Height())
						.levels(1)
						.format(texture->IsSRGB() ? filament::Texture::InternalFormat::SRGB8_A8 : filament::Texture::InternalFormat::RGBA8)
						.sampler(filament::Texture::Sampler::SAMPLER_2D)
						.build(*engine);

	if (!created)
	{
		return fallback;
	}

	const std::size_t rowBytes = static_cast<std::size_t>(texture->Width()) * 4;
	void* copy = std::malloc(pixels.size());
	for (std::uint32_t y = 0; y < texture->Height(); ++y)
	{
		std::memcpy(
			static_cast<std::uint8_t*>(copy) + static_cast<std::size_t>(texture->Height() - 1 - y) * rowBytes,
			pixels.data() + static_cast<std::size_t>(y) * rowBytes,
			rowBytes);
	}

	created->setImage(*engine, 0, filament::Texture::PixelBufferDescriptor(copy, pixels.size(), filament::Texture::Format::RGBA, filament::Texture::Type::UBYTE, [](void* buffer, std::size_t, void*) { std::free(buffer); }));

	cache[texture] = created;

	return created;
}

filament::TextureSampler CreateRepeatsLinearSampler() noexcept
{
	return filament::TextureSampler(
		filament::TextureSampler::MinFilter::LINEAR,
		filament::TextureSampler::MagFilter::LINEAR,
		filament::TextureSampler::WrapMode::REPEAT,
		filament::TextureSampler::WrapMode::REPEAT,
		filament::TextureSampler::WrapMode::REPEAT);
}

// Упаковывает атрибуты скиннинга в компактный буфер из 20 байт на вершину:
//   [0..3]  индексы костей как ubyte4    (BONE_INDICES, stride 20, offset 0)
//   [4..19] веса костей как float4       (BONE_WEIGHTS, stride 20, offset 4)
// Индексы из non-standard int-формата приводятся к ожидаемому Filament
// беззнаковому формату (uvec4), пустой bone id (-1) -> 0 с нулевым весом.
[[nodiscard]] std::vector<std::uint8_t> PackBoneAttributes(
	const std::uint8_t* vertices,
	const std::size_t vertexCount,
	const std::size_t vertexStride,
	const std::size_t indicesOffset,
	const std::size_t weightsOffset,
	const std::size_t boneCount)
{
	std::vector<std::uint8_t> packed(vertexCount * 20, 0);

	for (std::size_t i = 0; i < vertexCount; ++i)
	{
		const std::uint8_t* vertex = vertices + i * vertexStride;
		const auto* indices = reinterpret_cast<const std::int32_t*>(vertex + indicesOffset);
		const auto* weights = reinterpret_cast<const float*>(vertex + weightsOffset);

		std::uint8_t* dst = packed.data() + i * 20;
		for (int k = 0; k < 4; ++k)
		{
			const std::int32_t idx = indices[k];
			dst[k] = (idx >= 0 && static_cast<std::size_t>(idx) < boneCount) ? static_cast<std::uint8_t>(idx) : 0;
		}
		std::memcpy(dst + 4, weights, 4 * sizeof(float));
	}

	return packed;
}

} // namespace

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
		filament::MaterialInstance* materialInstance = nullptr;
		std::size_t boneCapacity = 0;
	};
	std::unordered_map<std::uint64_t, RenderResources> resourcesMap;

	filament::Material* litMaterial = nullptr;
	filament::Texture* whiteTexture = nullptr;
	filament::Texture* flatNormalTexture = nullptr;

	filament::IndirectLight* indirectLight = nullptr;
	float ambientLux = 0.0f;
	filament::math::float3 ambientColor{ 0.0f };

	std::unordered_map<const Texture*, filament::Texture*> textureCache;
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

	// m_impl->view->setPostProcessingEnabled(false);

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

	m_impl->litMaterial = filament::Material::Builder()
							  .package(LIT_RESOURCES_PACKAGE, LIT_RESOURCES_LIT_SIZE)
							  .build(*m_impl->engine);

	m_impl->whiteTexture = CreateSolidTexture(m_impl->engine, 255, 255, 255, 255);
	m_impl->flatNormalTexture = CreateSolidTexture(m_impl->engine, 128, 128, 255, 255);

	Resize(width, height);
}

void FilamentRenderBackend::Shutdown()
{
	if (m_impl->engine)
	{
		m_impl->uiHelper.reset();

		for (const auto& entity : m_impl->entityMap | std::views::values)
		{
			m_impl->engine->destroy(entity);
		}
		m_impl->entityMap.clear();

		for (auto& [vb, ib, inst, boneCapacity] : m_impl->resourcesMap | std::views::values)
		{
			if (inst && m_impl->litMaterial)
			{
				m_impl->engine->destroy(inst);
			}
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

		if (m_impl->indirectLight)
		{
			m_impl->scene->setIndirectLight(nullptr);
			m_impl->engine->destroy(m_impl->indirectLight);
			m_impl->indirectLight = nullptr;
		}

		if (m_impl->whiteTexture)
		{
			m_impl->engine->destroy(m_impl->whiteTexture);
			m_impl->whiteTexture = nullptr;
		}
		if (m_impl->flatNormalTexture)
		{
			m_impl->engine->destroy(m_impl->flatNormalTexture);
			m_impl->flatNormalTexture = nullptr;
		}
		for (const auto& tex : m_impl->textureCache | std::views::values)
		{
			if (tex)
			{
				m_impl->engine->destroy(tex);
			}
		}
		m_impl->textureCache.clear();
		if (m_impl->litMaterial)
		{
			m_impl->engine->destroy(m_impl->litMaterial);
			m_impl->litMaterial = nullptr;
		}

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

	const std::size_t vSize = mesh.vertexCount * mesh.vertexStride;
	void* vCopy = std::malloc(vSize);
	std::memcpy(vCopy, mesh.vertices, vSize);
	vb->setBufferAt(*m_impl->engine, 0, filament::VertexBuffer::BufferDescriptor(vCopy, vSize, [](void* buffer, size_t, void*) { std::free(buffer); }));

	filament::IndexBuffer* ib = filament::IndexBuffer::Builder()
									.indexCount(mesh.indexCount)
									.bufferType(mesh.is32BitIndices ? filament::IndexBuffer::IndexType::UINT : filament::IndexBuffer::IndexType::USHORT)
									.build(*m_impl->engine);

	const std::size_t iSize = mesh.indexCount * (mesh.is32BitIndices ? 4 : 2);
	void* iCopy = std::malloc(iSize);
	std::memcpy(iCopy, mesh.indices, iSize);
	ib->setBuffer(*m_impl->engine, filament::IndexBuffer::BufferDescriptor(iCopy, iSize, [](void* buffer, size_t, void*) { std::free(buffer); }));

	filament::math::float3 minBound{ std::numeric_limits<float>::max() };
	filament::math::float3 maxBound{ std::numeric_limits<float>::lowest() };

	const auto* vData = static_cast<const float*>(mesh.vertices);
	const std::size_t strideFloats = mesh.vertexStride / sizeof(float);

	for (std::size_t i = 0; i < mesh.vertexCount; ++i)
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

	filament::RenderableManager::Builder builder(1);
	builder
		.boundingBox(filament::Box{ (maxBound + minBound) * 0.5f, extent })
		.geometry(0, filament::RenderableManager::PrimitiveType::TRIANGLES, vb, ib)
		.culling(true);

	filament::MaterialInstance* materialInstance = nullptr;
	if (m_impl->litMaterial)
	{
		materialInstance = m_impl->litMaterial->createInstance();
		if (materialInstance)
		{
			const auto [r, g, b, a] = material.diffuse.ToFloat();
			materialInstance->setParameter("baseColorFactor", filament::math::float4{ r, g, b, a });

			const auto sampler = CreateRepeatsLinearSampler();
			materialInstance->setParameter("baseColorMap", GetOrCreateTexture(m_impl->engine, m_impl->textureCache, material.albedoTexture, m_impl->whiteTexture), sampler);
			materialInstance->setParameter("normalMap", m_impl->flatNormalTexture, sampler);

			builder.material(0, materialInstance);
		}
	}

	builder.build(*m_impl->engine, nativeEntity);

	m_impl->scene->addEntity(nativeEntity);
	m_impl->entityMap[id] = nativeEntity;
	m_impl->resourcesMap[id] = Impl::RenderResources{ vb, ib, materialInstance };

	return RenderEntityHandle{ id };
}

RenderEntityHandle FilamentRenderBackend::CreateAnimatedMesh(const MeshDataView& mesh, const MaterialDataView& material, const SkinDataView& skin)
{
	if (mesh.vertexCount == 0 || !mesh.vertices || mesh.indexCount == 0 || !mesh.indices)
	{
		return RenderEntityHandle{ 0 };
	}

	if (skin.boneCount == 0 || !skin.bones)
	{
		return CreateStaticMesh(mesh, material);
	}

	const utils::Entity nativeEntity = utils::EntityManager::get().create();
	const std::uint64_t id = ++m_impl->nextId;

	// Буфер 1: интерлив каноничного Vertex (позиция/цвет/uv/тангенс),
	// буфер 2: атрибуты скиннинга (см. PackBoneAttributes).
	filament::VertexBuffer* vb = filament::VertexBuffer::Builder()
									 .vertexCount(mesh.vertexCount)
									 .bufferCount(2)
									 .attribute(filament::VertexAttribute::POSITION, 0, filament::VertexBuffer::AttributeType::FLOAT3, 0, mesh.vertexStride)
									 .attribute(filament::VertexAttribute::COLOR, 0, filament::VertexBuffer::AttributeType::UBYTE4, 24, mesh.vertexStride)
									 .normalized(filament::VertexAttribute::COLOR)
									 .attribute(filament::VertexAttribute::UV0, 0, filament::VertexBuffer::AttributeType::FLOAT2, 28, mesh.vertexStride)
									 .attribute(filament::VertexAttribute::TANGENTS, 0, filament::VertexBuffer::AttributeType::FLOAT4, 72, mesh.vertexStride)
									 .attribute(filament::VertexAttribute::BONE_INDICES, 1, filament::VertexBuffer::AttributeType::UBYTE4, 0, 20)
									 .attribute(filament::VertexAttribute::BONE_WEIGHTS, 1, filament::VertexBuffer::AttributeType::FLOAT4, 4, 20)
									 .build(*m_impl->engine);

	const std::size_t vSize = mesh.vertexCount * mesh.vertexStride;
	void* vCopy = std::malloc(vSize);
	std::memcpy(vCopy, mesh.vertices, vSize);
	vb->setBufferAt(*m_impl->engine, 0, filament::VertexBuffer::BufferDescriptor(vCopy, vSize, [](void* buffer, size_t, void*) { std::free(buffer); }));

	const auto packedBones = PackBoneAttributes(
		static_cast<const std::uint8_t*>(mesh.vertices),
		mesh.vertexCount,
		mesh.vertexStride,
		skin.boneIndicesOffset,
		skin.boneWeightsOffset,
		skin.boneCount);

	void* bCopy = std::malloc(packedBones.size());
	std::memcpy(bCopy, packedBones.data(), packedBones.size());
	vb->setBufferAt(*m_impl->engine, 1, filament::VertexBuffer::BufferDescriptor(bCopy, packedBones.size(), [](void* buffer, size_t, void*) { std::free(buffer); }));

	filament::IndexBuffer* ib = filament::IndexBuffer::Builder()
									.indexCount(mesh.indexCount)
									.bufferType(mesh.is32BitIndices ? filament::IndexBuffer::IndexType::UINT : filament::IndexBuffer::IndexType::USHORT)
									.build(*m_impl->engine);

	const std::size_t iSize = mesh.indexCount * (mesh.is32BitIndices ? 4 : 2);
	void* iCopy = std::malloc(iSize);
	std::memcpy(iCopy, mesh.indices, iSize);
	ib->setBuffer(*m_impl->engine, filament::IndexBuffer::BufferDescriptor(iCopy, iSize, [](void* buffer, size_t, void*) { std::free(buffer); }));

	// Кости: Filament требует кратного 4 числа матриц, анимированная поза может
	// выходить за пределы box'а bind-позы, поэтому culling отключаем.
	std::size_t boneCapacity = std::max<std::size_t>(4, (skin.boneCount + 3) & ~std::size_t{ 3 });
	boneCapacity = std::min<std::size_t>(boneCapacity, 255);

	std::vector<filament::math::mat4f> boneMatrices(boneCapacity);
	const auto* srcBones = reinterpret_cast<const filament::math::mat4f*>(skin.bones);
	for (std::size_t i = 0; i < std::min(skin.boneCount, boneCapacity); ++i)
	{
		boneMatrices[i] = srcBones[i];
	}

	filament::RenderableManager::Builder builder(1);
	builder
		.boundingBox(filament::Box{ filament::math::float3{ 0.0f }, filament::math::float3{ 10.0f } })
		.geometry(0, filament::RenderableManager::PrimitiveType::TRIANGLES, vb, ib)
		.culling(false)
		.skinning(boneCapacity, boneMatrices.data());

	filament::MaterialInstance* materialInstance = nullptr;
	if (m_impl->litMaterial)
	{
		materialInstance = m_impl->litMaterial->createInstance();
		if (materialInstance)
		{
			const auto [r, g, b, a] = material.diffuse.ToFloat();
			materialInstance->setParameter("baseColorFactor", filament::math::float4{ r, g, b, a });

			const auto sampler = CreateRepeatsLinearSampler();
			materialInstance->setParameter("baseColorMap", GetOrCreateTexture(m_impl->engine, m_impl->textureCache, material.albedoTexture, m_impl->whiteTexture), sampler);
			materialInstance->setParameter("normalMap", m_impl->flatNormalTexture, sampler);

			builder.material(0, materialInstance);
		}
	}

	builder.build(*m_impl->engine, nativeEntity);

	m_impl->scene->addEntity(nativeEntity);
	m_impl->entityMap[id] = nativeEntity;
	m_impl->resourcesMap[id] = Impl::RenderResources{ vb, ib, materialInstance, boneCapacity };

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
		.falloff(light.falloff)
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
			if (resIt->second.materialInstance && m_impl->litMaterial)
			{
				m_impl->engine->destroy(resIt->second.materialInstance);
			}
			if (resIt->second.vb)
			{
				m_impl->engine->destroy(resIt->second.vb);
			}
			if (resIt->second.ib)
			{
				m_impl->engine->destroy(resIt->second.ib);
			}
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

void FilamentRenderBackend::UpdateBones(const RenderEntityHandle handle, const glm::mat4* bones, const std::size_t boneCount)
{
	if (!bones || boneCount == 0)
	{
		return;
	}

	const auto it = m_impl->entityMap.find(handle.id);
	if (it == m_impl->entityMap.end())
	{
		return;
	}

	const auto resIt = m_impl->resourcesMap.find(handle.id);
	if (resIt == m_impl->resourcesMap.end() || resIt->second.boneCapacity == 0)
	{
		return;
	}

	auto& rcm = m_impl->engine->getRenderableManager();
	const auto instance = rcm.getInstance(it->second);
	if (!instance)
	{
		return;
	}

	const std::size_t count = std::min(boneCount, resIt->second.boneCapacity);
	rcm.setBones(instance, reinterpret_cast<const filament::math::mat4f*>(bones), count, 0);
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
			lm.setFalloff(instance, light.falloff);
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
	m_impl->camera->setExposure(16.0f, 1.0f / 125.0f, std::max(camera.iso, 25.0f));
	m_impl->camera->setModelMatrix(*reinterpret_cast<const filament::math::mat4f*>(&inverseView));
}

void FilamentRenderBackend::SetSkybox(std::uint32_t cubemapID, std::uint32_t irradianceID) {}

void FilamentRenderBackend::SetAmbientLight(const float intensity, const Color color)
{
	if (!m_impl->engine || !m_impl->scene)
	{
		return;
	}

	const auto [r, g, b, a] = color.ToFloat();
	const filament::math::float3 linearColor = filament::Color::toLinear<filament::ACCURATE>({ r, g, b });

	if (m_impl->ambientLux == intensity && m_impl->ambientColor == linearColor)
	{
		return;
	}

	m_impl->ambientLux = intensity;
	m_impl->ambientColor = linearColor;

	if (m_impl->indirectLight)
	{
		m_impl->scene->setIndirectLight(nullptr);
		m_impl->engine->destroy(m_impl->indirectLight);
		m_impl->indirectLight = nullptr;
	}

	if (intensity <= 0.0f)
	{
		return;
	}

	constexpr float kY001OverCos = 12.566370614f; // 4*PI == 1 / (A[0] * Y00)
	const std::array sh{ linearColor * (intensity * kY001OverCos) };

	m_impl->indirectLight = filament::IndirectLight::Builder()
								.irradiance(1, sh.data())
								.intensity(1.0f)
								.build(*m_impl->engine);

	if (m_impl->indirectLight)
	{
		m_impl->scene->setIndirectLight(m_impl->indirectLight);
	}
}

void FilamentRenderBackend::SetClearColor(const Color color)
{
	if (m_impl->renderer)
	{
		const auto [r, g, b, a] = color.ToFloat();
		filament::Renderer::ClearOptions options;
		options.clearColor = { r, g, b, a };
		options.clear = true;
		options.discard = true;
		m_impl->renderer->setClearOptions(options);
	}
}

void FilamentRenderBackend::RenderUI(const float dt, std::function<void()> uiCallback)
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