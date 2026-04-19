#include <RenderCore/AnimatedModel.hpp>

#include <RenderCore/Material.hpp>

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include "RenderCore/Assets/AssetManager.hpp"

#include <tiny_gltf.h>

#include <filesystem>
#include <iostream>

namespace
{

template <typename T>
const uint8_t* GetAttributeData(const tinygltf::Model& model, const tinygltf::Primitive& primitive, const std::string& attribName, size_t& outCount, size_t& outStride)
{
	if (!primitive.attributes.contains(attribName))
	{
		return nullptr;
	}

	const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.at(attribName)];
	const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
	const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

	outCount = accessor.count;
	outStride = accessor.ByteStride(bufferView)
		? accessor.ByteStride(bufferView)
		: tinygltf::GetComponentSizeInBytes(accessor.componentType) * tinygltf::GetNumComponentsInType(accessor.type);

	return &buffer.data[bufferView.byteOffset + accessor.byteOffset];
}

bool DummyImageLoader(tinygltf::Image*, const int, std::string*,
	std::string*, int, int,
	const unsigned char*, int, void*)
{
	// Говорим библиотеке, что мы "успешно" обработали картинку,
	// чтобы она не прерывала парсинг остальной модели (геометрии и костей).
	return true;
}

} // namespace

namespace re
{

const std::vector<render::MeshPart>& AnimatedModel::GetParts() const
{
	return m_parts;
}

bool AnimatedModel::LoadFromFile(String const& filePath, const AssetManager* manager)
{
	tinygltf::Model gltfModel;
	tinygltf::TinyGLTF loader;
	std::string err, warn;

	const std::string pathStr = filePath.ToString();
	bool ret = false;

	loader.SetImageLoader(DummyImageLoader, nullptr);

	if (std::filesystem::path(pathStr).extension() == ".glb")
	{
		ret = loader.LoadBinaryFromFile(&gltfModel, &err, &warn, pathStr);
	}
	else
	{
		ret = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, pathStr);
	}

	if (!warn.empty())
		std::cout << "[glTF Warn]: " << warn << "\n";
	if (!err.empty())
		std::cerr << "[glTF Error]: " << err << "\n";
	if (!ret)
		return false;

	// ЛОКАЛЬНЫЙ КЭШ ТЕКСТУР: чтобы не дублировать текстуры и не спамить в AssetManager
	std::unordered_map<int, std::shared_ptr<Texture>> localTextureCache;

	// 1. Проходимся по всем мешам в файле
	for (const auto& mesh : gltfModel.meshes)
	{
		for (const auto& primitive : mesh.primitives)
		{
			render::MeshPart part;

			size_t vertexCount = 0, stride = 0;
			const uint8_t* posData = GetAttributeData<float>(gltfModel, primitive, "POSITION", vertexCount, stride);
			if (!posData)
				continue;

			part.vertices.resize(vertexCount);

			// Позиции
			for (size_t i = 0; i < vertexCount; ++i)
			{
				auto pos = reinterpret_cast<const float*>(posData + i * stride);
				part.vertices[i].position = { pos[0], pos[1], pos[2] };
			}

			// Нормали
			size_t count, normStride;
			if (const uint8_t* normData = GetAttributeData<float>(gltfModel, primitive, "NORMAL", count, normStride))
			{
				for (size_t i = 0; i < count; ++i)
				{
					auto norm = reinterpret_cast<const float*>(normData + i * normStride);
					part.vertices[i].normal = { norm[0], norm[1], norm[2] };
				}
			}

			// УМНАЯ ЗАГРУЗКА UV-КООРДИНАТ (Защита от мусора и переворота)
			size_t uvStride;
			if (const uint8_t* uvData = GetAttributeData<float>(gltfModel, primitive, "TEXCOORD_0", count, uvStride))
			{
				const int uvCompType = gltfModel.accessors[primitive.attributes.at("TEXCOORD_0")].componentType;
				for (size_t i = 0; i < count; ++i)
				{
					const uint8_t* raw = uvData + i * uvStride;
					float u = 0.0f, v = 0.0f;

					if (uvCompType == TINYGLTF_COMPONENT_TYPE_FLOAT)
					{
						const float* f = reinterpret_cast<const float*>(raw);
						u = f[0];
						v = f[1];
					}
					else if (uvCompType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
					{
						const uint16_t* s = reinterpret_cast<const uint16_t*>(raw);
						u = s[0] / 65535.0f;
						v = s[1] / 65535.0f;
					}
					else if (uvCompType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
					{
						const uint8_t* b = reinterpret_cast<const uint8_t*>(raw);
						u = b[0] / 255.0f;
						v = b[1] / 255.0f;
					}

					// ИСПРАВЛЕНИЕ ДЛЯ OPENGL: Инвертируем V!
					part.vertices[i].texCoord = { u, 1.0f - v };
				}
			}

			// Кости (JOINTS_0)
			size_t jointStride;
			if (const uint8_t* jointData = GetAttributeData<uint16_t>(gltfModel, primitive, "JOINTS_0", count, jointStride))
			{
				const int componentType = gltfModel.accessors[primitive.attributes.at("JOINTS_0")].componentType;
				for (size_t i = 0; i < count; ++i)
				{
					if (componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
					{
						auto joints = reinterpret_cast<const uint16_t*>(jointData + i * jointStride);
						part.vertices[i].boneIDs = { joints[0], joints[1], joints[2], joints[3] };
					}
					else if (componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
					{
						auto joints = reinterpret_cast<const uint8_t*>(jointData + i * jointStride);
						part.vertices[i].boneIDs = { joints[0], joints[1], joints[2], joints[3] };
					}
				}
			}

			// Веса (WEIGHTS_0)
			size_t weightStride;
			if (const uint8_t* weightData = GetAttributeData<float>(gltfModel, primitive, "WEIGHTS_0", count, weightStride))
			{
				for (size_t i = 0; i < count; ++i)
				{
					auto weights = reinterpret_cast<const float*>(weightData + i * weightStride);
					part.vertices[i].boneWeights = { weights[0], weights[1], weights[2], weights[3] };
				}
			}

			// Индексы
			if (primitive.indices >= 0)
			{
				const tinygltf::Accessor& indexAccessor = gltfModel.accessors[primitive.indices];
				const tinygltf::BufferView& bufferView = gltfModel.bufferViews[indexAccessor.bufferView];
				const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];

				const uint8_t* data = &buffer.data[bufferView.byteOffset + indexAccessor.byteOffset];
				part.indices.resize(indexAccessor.count);

				for (size_t i = 0; i < indexAccessor.count; ++i)
				{
					if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
						part.indices[i] = reinterpret_cast<const uint32_t*>(data)[i];
					else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
						part.indices[i] = reinterpret_cast<const uint16_t*>(data)[i];
					else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
						part.indices[i] = reinterpret_cast<const uint8_t*>(data)[i];
				}
			}

			// --- Материалы и Кэширование Текстур ---
			part.material = Material();
			if (primitive.material >= 0)
			{
				const tinygltf::Material& gltfMat = gltfModel.materials[primitive.material];

				// Конвертируем PBR
				if (gltfMat.pbrMetallicRoughness.baseColorFactor.size() == 4)
				{
					const auto& c = gltfMat.pbrMetallicRoughness.baseColorFactor;
					part.material.diffuse = Color{
						static_cast<uint8_t>(c[0] * 255.0f),
						static_cast<uint8_t>(c[1] * 255.0f),
						static_cast<uint8_t>(c[2] * 255.0f),
						static_cast<uint8_t>(c[3] * 255.0f)
					};
					part.material.ambient = part.material.diffuse;
				}

				const int baseColorTexIndex = gltfMat.pbrMetallicRoughness.baseColorTexture.index;
				if (baseColorTexIndex >= 0)
				{
					// Проверяем, не загружали ли мы эту текстуру минуту назад для другой части модели
					if (localTextureCache.contains(baseColorTexIndex))
					{
						part.material.texture = localTextureCache[baseColorTexIndex];
					}
					else
					{
						const tinygltf::Texture& gltfTex = gltfModel.textures[baseColorTexIndex];
						const tinygltf::Image& gltfImg = gltfModel.images[gltfTex.source];

						if (!gltfImg.uri.empty())
						{
							// Внешний файл (грязная загрузка через глобальный AssetManager)
							std::filesystem::path modelPath(pathStr);
							std::filesystem::path texPath = modelPath.parent_path() / gltfImg.uri;
							part.material.texture = manager->Get<Texture>(String(texPath));
						}
						else if (gltfImg.bufferView >= 0)
						{
							// Встроенная текстура
							const tinygltf::BufferView& bufferView = gltfModel.bufferViews[gltfImg.bufferView];
							const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];

							const unsigned char* rawImageData = &buffer.data[bufferView.byteOffset];
							const std::size_t rawImageSize = bufferView.byteLength;

							if (auto t = std::make_shared<Texture>(); t->LoadFromMemory(rawImageData, rawImageSize))
							{
								part.material.texture = t;
								localTextureCache[baseColorTexIndex] = t; // Сохраняем в кэш!
							}
						}
					}
				}
			}

			m_parts.push_back(std::move(part));
		}
	}

	return true;
}

} // namespace re