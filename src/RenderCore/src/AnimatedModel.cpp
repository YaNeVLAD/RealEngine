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

			std::size_t jointsCount, jointsStride;
			if (const std::uint8_t* jointsData = GetAttributeData<float>(gltfModel, primitive, "JOINTS_0", jointsCount, jointsStride))
			{
				const int componentType = gltfModel.accessors[primitive.attributes.at("JOINTS_0")].componentType;
				for (std::size_t i = 0; i < jointsCount; ++i)
				{
					const std::uint8_t* raw = jointsData + i * jointsStride;

					// В glTF индексы костей обычно хранятся как UNSIGNED_SHORT или UNSIGNED_BYTE
					if (componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
					{
						auto j = reinterpret_cast<const std::uint16_t*>(raw);
						part.vertices[i].boneIDs = { j[0], j[1], j[2], j[3] };
					}
					else if (componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
					{
						auto j = raw;
						part.vertices[i].boneIDs = { j[0], j[1], j[2], j[3] };
					}
				}
			}

			std::size_t weightsCount, weightsStride;
			if (const std::uint8_t* weightsData = GetAttributeData<float>(gltfModel, primitive, "WEIGHTS_0", weightsCount, weightsStride))
			{
				const int componentType = gltfModel.accessors[primitive.attributes.at("WEIGHTS_0")].componentType;
				for (std::size_t i = 0; i < weightsCount; ++i)
				{
					const std::uint8_t* raw = weightsData + i * weightsStride;

					if (componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
					{
						auto w = reinterpret_cast<const float*>(raw);
						part.vertices[i].boneWeights = { w[0], w[1], w[2], w[3] };
					}
					else if (componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
					{
						auto w = reinterpret_cast<const std::uint16_t*>(raw);
						part.vertices[i].boneWeights = { w[0] / 65535.0f, w[1] / 65535.0f, w[2] / 65535.0f, w[3] / 65535.0f };
					}
					else if (componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
					{
						auto w = raw;
						part.vertices[i].boneWeights = { w[0] / 255.0f, w[1] / 255.0f, w[2] / 255.0f, w[3] / 255.0f };
					}
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
			part.material.workflow = re::MaterialWorkflow::PBR;
			if (primitive.material >= 0)
			{
				auto LoadTextureFromIndex = [&](const int texIndex, const bool srgb) -> std::shared_ptr<re::Texture> {
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

					auto result = std::make_shared<re::Texture>();
					bool loaded = false;
					re::String texCacheKey;

					if (!gltfImg.uri.empty())
					{
						const std::filesystem::path modelPath(pathStr);
						const std::filesystem::path texPath = modelPath.parent_path() / gltfImg.uri;
						texCacheKey = re::String(texPath);

						loaded = result->LoadFromFileSRGB(texCacheKey, srgb);
					}
					else if (gltfImg.bufferView >= 0)
					{
						const tinygltf::BufferView& bufferView = gltfModel.bufferViews[gltfImg.bufferView];
						const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];
						const unsigned char* rawImageData = &buffer.data[bufferView.byteOffset];
						const std::size_t rawImageSize = bufferView.byteLength;

						texCacheKey = re::String(pathStr + "_tex_" + std::to_string(texIndex));

						loaded = result->LoadFromMemorySRGB(rawImageData, rawImageSize, srgb);
					}

					if (loaded)
					{
						textureCache[texIndex] = result;
						const_cast<re::AssetManager*>(manager)->Add(texCacheKey, result);

						return result;
					}

					return nullptr;
				};

				const tinygltf::Material& gltfMat = gltfModel.materials[primitive.material];
				auto& mat = part.material;

				mat.workflow = re::MaterialWorkflow::PBR;

				mat.metallicFactor = static_cast<float>(gltfMat.pbrMetallicRoughness.metallicFactor);
				mat.roughnessFactor = static_cast<float>(gltfMat.pbrMetallicRoughness.roughnessFactor);

				int albedoIdx = gltfMat.pbrMetallicRoughness.baseColorTexture.index;
				mat.albedoMap = LoadTextureFromIndex(albedoIdx, true);

				int mrIdx = gltfMat.pbrMetallicRoughness.metallicRoughnessTexture.index;
				mat.metallicRoughnessMap = LoadTextureFromIndex(mrIdx, false);

				int normIdx = gltfMat.normalTexture.index;
				mat.normalMap = LoadTextureFromIndex(normIdx, false);

				int emisIdx = gltfMat.emissiveTexture.index;
				mat.emissionMap = LoadTextureFromIndex(emisIdx, true);

				int aoIdx = gltfMat.occlusionTexture.index;
				mat.ambientOcclusionMap = LoadTextureFromIndex(aoIdx, false);
			}

			outParts.push_back(std::move(part));
		}
	}

	for (int childIndex : gltfNode.children)
	{
		ParseNodeRecursive(gltfModel, childIndex, globalTransform, outParts, textureCache, manager, pathStr);
	}
}

void PrintBoneRecursive(const std::vector<re::render::Bone>& skeleton, int boneIndex, int depth);

void PrintSkeleton(const std::vector<re::render::Bone>& skeleton)
{
	if (skeleton.empty())
	{
		std::cout << "Model has no skeleton.\n";
		return;
	}

	std::cout << "--- Skeleton Hierarchy ---\n";

	for (std::size_t i = 0; i < skeleton.size(); ++i)
	{
		if (skeleton[i].parentIndex == re::render::Bone::ROOT_BONE_IDX)
		{
			PrintBoneRecursive(skeleton, static_cast<int>(i), 0);
		}
	}
	std::cout << "--------------------------\n";
}

void PrintBoneRecursive(const std::vector<re::render::Bone>& skeleton, const int boneIndex, const int depth)
{
	for (int i = 0; i < depth; ++i)
	{
		std::cout << "  ";
	}

	std::cout << "|- [" << boneIndex << "] " << skeleton[boneIndex].name << "\n";

	for (std::size_t i = 0; i < skeleton.size(); ++i)
	{
		if (skeleton[i].parentIndex == boneIndex)
		{
			PrintBoneRecursive(skeleton, static_cast<int>(i), depth + 1);
		}
	}
}

} // namespace

namespace re
{

const std::vector<render::MeshPart>& AnimatedModel::Parts() const
{
	return m_parts;
}

const std::vector<render::Bone>& AnimatedModel::Skeleton() const
{
	return m_skeleton;
}

const std::vector<render::Animation>& AnimatedModel::Animations() const
{
	return m_animations;
}

bool AnimatedModel::LoadFromFile(String const& filePath, const AssetManager* manager)
{
	using namespace re::render;

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

	for (const tinygltf::Scene& scene = gltfModel.scenes[sceneIndex]; const int nodeIndex : scene.nodes)
	{
		ParseNodeRecursive(gltfModel, nodeIndex, glm::mat4(1.0f), m_parts, localTextureCache, manager, pathStr);
	}

	// Хэш-таблица для быстрого поиска: индекс узла glTF -> индекс кости в нашем массиве
	std::unordered_map<int, int> nodeToBoneMap;
	if (!gltfModel.skins.empty())
	{
		if (gltfModel.skins.size() > 1)
		{
			RE_LOG_WARN("Multiple skins detected in {}. All shins except for the first one will be ignored", filePath.ToString());
		}
		const tinygltf::Skin& skin = gltfModel.skins[0];
		m_skeleton.resize(skin.joints.size());

		for (std::size_t i = 0; i < skin.joints.size(); ++i)
		{
			nodeToBoneMap[skin.joints[i]] = static_cast<int>(i);
		}

		// Читаем Inverse Bind Matrices
		const float* ibmData = nullptr;
		if (skin.inverseBindMatrices >= 0)
		{
			const tinygltf::Accessor& accessor = gltfModel.accessors[skin.inverseBindMatrices];
			const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
			const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];
			ibmData = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
		}

		// Заполняем кости
		for (std::size_t i = 0; i < skin.joints.size(); ++i)
		{
			int nodeIndex = skin.joints[i];
			const tinygltf::Node& node = gltfModel.nodes[nodeIndex];

			m_skeleton[i].name = node.name;

			// Записываем Inverse Bind Matrix (если есть, иначе оставляем единичную)
			if (ibmData)
			{
				m_skeleton[i].inverseBindMatrix = glm::make_mat4(ibmData + i * 16);
			}

			// Вычисляем базовый локальный трансформ кости (T * R * S)
			glm::vec3 translation(0.0f);
			glm::quat rotation(1.0f, 0.0f, 0.0f, 0.0f);
			glm::vec3 scale(1.0f);

			if (node.translation.size() == 3)
			{
				translation = glm::make_vec3(node.translation.data());
			}
			if (node.rotation.size() == 4)
			{
				rotation = glm::quat(static_cast<float>(node.rotation[3]), static_cast<float>(node.rotation[0]), static_cast<float>(node.rotation[1]), static_cast<float>(node.rotation[2]));
			}
			if (node.scale.size() == 3)
			{
				scale = glm::make_vec3(node.scale.data());
			}

			m_skeleton[i].localTransform = glm::translate(glm::mat4(1.0f), translation) * glm::mat4_cast(rotation) * glm::scale(glm::mat4(1.0f), scale);
			m_skeleton[i].globalTransform = m_skeleton[i].localTransform; // Пока глобальный равен локальному

			// Ищем детей этой кости и устанавливаем им parentIndex
			for (int childNodeIndex : node.children)
			{
				if (nodeToBoneMap.contains(childNodeIndex))
				{
					int childBoneIndex = nodeToBoneMap[childNodeIndex];
					m_skeleton[childBoneIndex].parentIndex = static_cast<int>(i);
				}
			}
		}
	}

	for (const tinygltf::Animation& gltfAnim : gltfModel.animations)
	{
		Animation anim;
		anim.name = gltfAnim.name;
		float maxTime = 0.0f;

		for (const tinygltf::AnimationChannel& channel : gltfAnim.channels)
		{
			int nodeIndex = channel.target_node;

			// Если анимация направлена не на кость из нашего скелета - пропускаем
			if (!nodeToBoneMap.contains(nodeIndex))
			{
				continue;
			}
			int boneIndex = nodeToBoneMap[nodeIndex];

			const tinygltf::AnimationSampler& sampler = gltfAnim.samplers[channel.sampler];

			// --- Читаем ВРЕМЯ (Inputs) ---
			const tinygltf::Accessor& inputAccessor = gltfModel.accessors[sampler.input];
			const tinygltf::BufferView& inputView = gltfModel.bufferViews[inputAccessor.bufferView];
			const tinygltf::Buffer& inputBuffer = gltfModel.buffers[inputView.buffer];

			const unsigned char* inputData = &inputBuffer.data[inputView.byteOffset + inputAccessor.byteOffset];
			std::size_t inputStride = inputAccessor.ByteStride(inputView) ? inputAccessor.ByteStride(inputView) : sizeof(float);

			std::vector<float> times(inputAccessor.count);
			for (std::size_t i = 0; i < inputAccessor.count; ++i)
			{
				times[i] = *reinterpret_cast<const float*>(inputData + i * inputStride);
				if (times[i] > maxTime)
				{
					maxTime = times[i];
				}
			}

			// --- Читаем ЗНАЧЕНИЯ (Outputs) ---
			const tinygltf::Accessor& outputAccessor = gltfModel.accessors[sampler.output];
			const tinygltf::BufferView& outputView = gltfModel.bufferViews[outputAccessor.bufferView];
			const tinygltf::Buffer& outputBuffer = gltfModel.buffers[outputView.buffer];

			const unsigned char* outputData = &outputBuffer.data[outputView.byteOffset + outputAccessor.byteOffset];
			std::size_t outputStride = outputAccessor.ByteStride(outputView);
			if (outputStride == 0)
			{
				outputStride = (channel.target_path == "rotation") ? sizeof(float) * 4 : sizeof(float) * 3;
			}

			BoneAnimation& boneAnim = anim.boneAnimations[boneIndex];

			if (channel.target_path == "translation")
			{
				for (std::size_t i = 0; i < inputAccessor.count; ++i)
				{
					const float* val = reinterpret_cast<const float*>(outputData + i * outputStride);
					boneAnim.positions.push_back({ times[i], glm::vec3(val[0], val[1], val[2]) });
				}
			}
			else if (channel.target_path == "rotation")
			{
				for (std::size_t i = 0; i < inputAccessor.count; ++i)
				{
					const float* val = reinterpret_cast<const float*>(outputData + i * outputStride);
					// ВНИМАНИЕ: glTF хранит кватернионы как [X, Y, Z, W], а конструктор glm::quat ожидает (W, X, Y, Z)
					boneAnim.rotations.push_back({ times[i], glm::quat(val[3], val[0], val[1], val[2]) });
				}
			}
			else if (channel.target_path == "scale")
			{
				for (std::size_t i = 0; i < inputAccessor.count; ++i)
				{
					const float* val = reinterpret_cast<const float*>(outputData + i * outputStride);
					boneAnim.scales.push_back({ times[i], glm::vec3(val[0], val[1], val[2]) });
				}
			}
		}

		anim.duration = maxTime;
		m_animations.push_back(anim);
	}

	PrintSkeleton(m_skeleton);

	return true;
}

} // namespace re