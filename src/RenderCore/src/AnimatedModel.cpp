#include <RenderCore/AnimatedModel.hpp>
#include <RenderCore/Assets/AssetManager.hpp>
#include <RenderCore/Material.hpp>

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include <tiny_gltf.h>

#include <filesystem>
#include <glm/gtc/type_ptr.hpp>
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
	outStride = accessor.ByteStride(bufferView) ? accessor.ByteStride(bufferView) : tinygltf::GetComponentSizeInBytes(accessor.componentType) * tinygltf::GetNumComponentsInType(accessor.type);

	return &buffer.data[bufferView.byteOffset + accessor.byteOffset];
}

bool DummyImageLoader(tinygltf::Image*, const int, std::string*, std::string*, int, int, const unsigned char*, int, void*) { return true; }

using TextureCache = std::unordered_map<int, std::shared_ptr<re::Texture>>;

void ParseNodeRecursive(
	const tinygltf::Model& gltfModel,
	int nodeIndex,
	const glm::mat4& parentTransform,
	std::vector<re::render::MeshPart>& outParts,
	TextureCache& textureCache,
	const re::AssetManager* manager,
	const std::string& pathStr)
{
	const tinygltf::Node& gltfNode = gltfModel.nodes[nodeIndex];

	glm::mat4 localTransform;
	if (gltfNode.matrix.size() == 16)
	{
		localTransform = glm::make_mat4(gltfNode.matrix.data());
	}
	else
	{
		glm::vec3 translation(0.0f);
		glm::quat rotation(1.0f, 0.0f, 0.0f, 0.0f);
		glm::vec3 scale(1.0f);

		if (gltfNode.translation.size() == 3)
		{
			translation = glm::make_vec3(gltfNode.translation.data());
		}
		if (gltfNode.rotation.size() == 4)
		{
			rotation = glm::quat(static_cast<float>(gltfNode.rotation[3]), static_cast<float>(gltfNode.rotation[0]), static_cast<float>(gltfNode.rotation[1]), static_cast<float>(gltfNode.rotation[2]));
		}
		if (gltfNode.scale.size() == 3)
		{
			scale = glm::make_vec3(gltfNode.scale.data());
		}

		localTransform = glm::translate(glm::mat4(1.0f), translation) * glm::mat4_cast(rotation) * glm::scale(glm::mat4(1.0f), scale);
	}

	auto globalTransform = parentTransform * localTransform;
	auto normalTransform = glm::mat3(glm::transpose(glm::inverse(globalTransform)));

	if (gltfNode.mesh >= 0)
	{
		for (const tinygltf::Mesh& mesh = gltfModel.meshes[gltfNode.mesh]; const auto& primitive : mesh.primitives)
		{
			re::render::MeshPart part;
			std::size_t vertexCount = 0, stride = 0;

			const std::uint8_t* posData = GetAttributeData<float>(gltfModel, primitive, "POSITION", vertexCount, stride);
			if (!posData)
			{
				continue;
			}

			part.vertices.resize(vertexCount);
			for (std::size_t i = 0; i < vertexCount; ++i)
			{
				auto pos = reinterpret_cast<const float*>(posData + i * stride);

				glm::vec4 worldPos = globalTransform * glm::vec4(pos[0], pos[1], pos[2], 1.0f);
				part.vertices[i].position = { worldPos.x, worldPos.y, worldPos.z };
			}

			std::size_t count, normStride;
			if (const std::uint8_t* normData = GetAttributeData<float>(gltfModel, primitive, "NORMAL", count, normStride))
			{
				for (std::size_t i = 0; i < count; ++i)
				{
					auto norm = reinterpret_cast<const float*>(normData + i * normStride);
					glm::vec3 worldNormal = glm::normalize(normalTransform * glm::vec3(norm[0], norm[1], norm[2]));
					part.vertices[i].normal = { worldNormal.x, worldNormal.y, worldNormal.z };
				}
			}

			size_t tangStride;
			if (const uint8_t* tangData = GetAttributeData<float>(gltfModel, primitive, "TANGENT", count, tangStride))
			{
				for (size_t i = 0; i < count; ++i)
				{
					auto tang = reinterpret_cast<const float*>(tangData + i * tangStride);
					glm::vec3 worldTang = glm::normalize(normalTransform * glm::vec3(tang[0], tang[1], tang[2]));
					part.vertices[i].tangent = { worldTang.x, worldTang.y, worldTang.z, tang[3] };
				}
			}

			std::size_t uvStride;
			if (const std::uint8_t* uvData = GetAttributeData<float>(gltfModel, primitive, "TEXCOORD_0", count, uvStride))
			{
				const int uvCompType = gltfModel.accessors[primitive.attributes.at("TEXCOORD_0")].componentType;
				for (std::size_t i = 0; i < count; ++i)
				{
					const std::uint8_t* raw = uvData + i * uvStride;
					float u = 0.0f, v = 0.0f;

					if (uvCompType == TINYGLTF_COMPONENT_TYPE_FLOAT)
					{
						auto f = reinterpret_cast<const float*>(raw);
						u = f[0];
						v = f[1];
					}
					else if (uvCompType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
					{
						auto s = reinterpret_cast<const uint16_t*>(raw);
						u = s[0] / 65535.0f;
						v = s[1] / 65535.0f;
					}
					else if (uvCompType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
					{
						auto b = raw;
						u = b[0] / 255.0f;
						v = b[1] / 255.0f;
					}
					part.vertices[i].texCoord = { u, 1.0f - v };
				}
			}

			if (primitive.indices >= 0)
			{
				const tinygltf::Accessor& indexAccessor = gltfModel.accessors[primitive.indices];
				const tinygltf::BufferView& bufferView = gltfModel.bufferViews[indexAccessor.bufferView];
				const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];

				const std::uint8_t* data = &buffer.data[bufferView.byteOffset + indexAccessor.byteOffset];
				std::size_t iStride = indexAccessor.ByteStride(bufferView);
				if (iStride == 0)
				{
					iStride = tinygltf::GetComponentSizeInBytes(indexAccessor.componentType);
				}

				part.indices.resize(indexAccessor.count);
				for (std::size_t i = 0; i < indexAccessor.count; ++i)
				{
					const std::uint8_t* element = data + i * iStride;
					if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
					{
						part.indices[i] = *reinterpret_cast<const std::uint32_t*>(element);
					}
					else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
					{
						part.indices[i] = *reinterpret_cast<const std::uint16_t*>(element);
					}
					else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
					{
						part.indices[i] = *element;
					}
				}
			}
			else
			{
				part.indices.resize(vertexCount);
				for (std::uint32_t i = 0; i < vertexCount; ++i)
				{
					part.indices[i] = i;
				}
			}

			part.material = re::Material();
			if (primitive.material >= 0)
			{
				const tinygltf::Material& gltfMat = gltfModel.materials[primitive.material];
				if (gltfMat.pbrMetallicRoughness.baseColorFactor.size() == 4)
				{
					const auto& c = gltfMat.pbrMetallicRoughness.baseColorFactor;
					part.material.diffuse = re::Color{
						static_cast<uint8_t>(c[0] * 255.0f), static_cast<uint8_t>(c[1] * 255.0f),
						static_cast<uint8_t>(c[2] * 255.0f), static_cast<uint8_t>(c[3] * 255.0f)
					};
					part.material.ambient = part.material.diffuse;
				}

				auto LoadTextureFromIndex = [&](const int texIndex) -> std::shared_ptr<re::Texture> {
					if (texIndex < 0)
					{
						return nullptr;
					}
					if (textureCache.contains(texIndex))
					{
						return textureCache[texIndex];
					}

					const tinygltf::Texture& gltfTex = gltfModel.textures[texIndex];
					const tinygltf::Image& gltfImg = gltfModel.images[gltfTex.source];

					std::shared_ptr<re::Texture> result = nullptr;
					if (!gltfImg.uri.empty())
					{
						const std::filesystem::path modelPath(pathStr);
						const std::filesystem::path texPath = modelPath.parent_path() / gltfImg.uri;
						result = manager->Get<re::Texture>(re::String(texPath));
					}
					else if (gltfImg.bufferView >= 0)
					{
						const tinygltf::BufferView& bufferView = gltfModel.bufferViews[gltfImg.bufferView];
						const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];
						const unsigned char* rawImageData = &buffer.data[bufferView.byteOffset];
						const std::size_t rawImageSize = bufferView.byteLength;

						result = std::make_shared<re::Texture>();
						if (!result->LoadFromMemory(rawImageData, rawImageSize))
						{
							result = nullptr;
						}
					}

					if (result)
					{
						textureCache[texIndex] = result;
					}

					return result;
				};

				part.material.albedoMap = LoadTextureFromIndex(gltfMat.pbrMetallicRoughness.baseColorTexture.index);
				part.material.normalMap = LoadTextureFromIndex(gltfMat.normalTexture.index);
				part.material.emissionMap = LoadTextureFromIndex(gltfMat.emissiveTexture.index);
			}

			outParts.push_back(std::move(part));
		}
	}

	for (int childIndex : gltfNode.children)
	{
		ParseNodeRecursive(gltfModel, childIndex, globalTransform, outParts, textureCache, manager, pathStr);
	}
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
	{
		std::cout << "[glTF Warn]: " << warn << "\n";
	}
	if (!err.empty())
	{
		std::cerr << "[glTF Error]: " << err << "\n";
	}
	if (!ret)
	{
		return false;
	}

	std::unordered_map<int, std::shared_ptr<Texture>> localTextureCache;

	const int sceneIndex = gltfModel.defaultScene > -1 ? gltfModel.defaultScene : 0;
	const tinygltf::Scene& scene = gltfModel.scenes[sceneIndex];

	for (const int nodeIndex : scene.nodes)
	{
		ParseNodeRecursive(gltfModel, nodeIndex, glm::mat4(1.0f), m_parts, localTextureCache, manager, pathStr);
	}

	return true;
}

} // namespace re