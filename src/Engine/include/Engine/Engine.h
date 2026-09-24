#pragma once

#include <Engine/Export.hpp>

#include <stdint.h> // NOLINT(*-deprecated-headers)

#ifdef __cplusplus
extern "C" {
#endif

#define RE_ENGINE_API_VERSION 1u

typedef void(RE_CALL* ReEngine_LogCallback)(int32_t level, const char* messageUtf8, void* userdata);

#pragma pack(push, 8)

typedef enum ReEngine_Status
{
	RE_ENGINE_OK = 0,
	RE_ENGINE_NOT_INITIALIZED = 1,
	RE_ENGINE_INVALID_ARGUMENT = 2,
	RE_ENGINE_INVALID_HANDLE = 3,
	RE_ENGINE_VERSION_MISMATCH = 4,
	RE_ENGINE_FAILED = 5,
	RE_ENGINE_ALREADY_INITIALIZED = 6
} ReEngine_Status;

typedef enum ReEngine_LogLevel
{
	RE_ENGINE_LOG_INFO = 0,
	RE_ENGINE_LOG_WARNING = 1,
	RE_ENGINE_LOG_ERROR = 2
} ReEngine_LogLevel;

typedef enum ReEngine_PrimitiveKind
{
	RE_ENGINE_PRIMITIVE_CUBE = 0,
	RE_ENGINE_PRIMITIVE_SPHERE = 1
} ReEngine_PrimitiveKind;

typedef enum ReEngine_FieldType
{
	RE_ENGINE_FIELD_BOOL = 0,
	RE_ENGINE_FIELD_INT32 = 1,
	RE_ENGINE_FIELD_FLOAT = 2,
	RE_ENGINE_FIELD_FLOAT2 = 3,
	RE_ENGINE_FIELD_FLOAT3 = 4,
	RE_ENGINE_FIELD_COLOR = 5
} ReEngine_FieldType;

typedef struct ReEngine_InitArgs
{
	uint32_t version;
	ReEngine_LogCallback logCallback;
	void* logUserdata;
	const char* assetsDirUtf8;
	uint32_t flags;
} ReEngine_InitArgs;

typedef struct ReEngine_ComponentFieldInfo
{
	char name[48];
	int32_t type;
	uint32_t size;
} ReEngine_ComponentFieldInfo;

#pragma pack(pop)

RE_ENGINE_API int32_t RE_CALL ReEngine_GetVersion(void);

RE_ENGINE_API int32_t RE_CALL ReEngine_Initialize(const ReEngine_InitArgs* args);
RE_ENGINE_API void RE_CALL ReEngine_Shutdown(void);
RE_ENGINE_API int32_t RE_CALL ReEngine_Update(float deltaSeconds);
RE_ENGINE_API void RE_CALL ReEngine_SetLogCallback(ReEngine_LogCallback callback, void* userdata);

RE_ENGINE_API uint64_t RE_CALL ReEngine_ViewportCreate(uint64_t hwnd, uint32_t width, uint32_t height);
RE_ENGINE_API int32_t RE_CALL ReEngine_ViewportRender(uint64_t viewport);
RE_ENGINE_API int32_t RE_CALL ReEngine_ViewportResize(uint64_t viewport, uint32_t width, uint32_t height);
RE_ENGINE_API void RE_CALL ReEngine_ViewportDestroy(uint64_t viewport);
RE_ENGINE_API int32_t RE_CALL ReEngine_ViewportSetClearColor(uint64_t viewport, uint32_t rgba8888);

RE_ENGINE_API uint64_t RE_CALL ReEngine_SceneCreateEntity(void);
RE_ENGINE_API int32_t RE_CALL ReEngine_SceneIsEntityValid(uint64_t entity);
RE_ENGINE_API void RE_CALL ReEngine_SceneDestroyEntity(uint64_t entity);
RE_ENGINE_API uint32_t RE_CALL ReEngine_SceneGetEntityCount(void);
RE_ENGINE_API int32_t RE_CALL ReEngine_SceneGetEntities(uint64_t* outIds, uint32_t capacity, uint32_t* outTotal);
RE_ENGINE_API uint32_t RE_CALL ReEngine_SceneClearEntities(void);
RE_ENGINE_API uint64_t RE_CALL ReEngine_SceneSpawnPrimitive(int32_t kind, uint32_t rgba8888);
RE_ENGINE_API void RE_CALL ReEngine_SceneConfirmChanges(void);

RE_ENGINE_API uint64_t RE_CALL ReEngine_EntityGetComponentMask(uint64_t entity);
RE_ENGINE_API int32_t RE_CALL ReEngine_EntityAddComponent(uint64_t entity, int32_t component);
RE_ENGINE_API int32_t RE_CALL ReEngine_EntityRemoveComponent(uint64_t entity, int32_t component);
RE_ENGINE_API int32_t RE_CALL ReEngine_EntityGetFieldData(uint64_t entity, int32_t component, int32_t fieldIndex, void* outData, uint32_t maxBytes, uint32_t* outBytes);
RE_ENGINE_API int32_t RE_CALL ReEngine_EntitySetFieldData(uint64_t entity, int32_t component, int32_t fieldIndex, const void* data, uint32_t bytes);
RE_ENGINE_API int32_t RE_CALL ReEngine_EntityGetName(uint64_t entity, char* outName, uint32_t capacity, uint32_t* outNeeded);
RE_ENGINE_API int32_t RE_CALL ReEngine_EntitySetName(uint64_t entity, const char* nameUtf8);
RE_ENGINE_API int32_t RE_CALL ReEngine_ComponentGetFieldCount(int32_t component);
RE_ENGINE_API int32_t RE_CALL ReEngine_ComponentGetFieldInfo(int32_t component, int32_t fieldIndex, ReEngine_ComponentFieldInfo* outInfo);

RE_ENGINE_API uint32_t RE_CALL ReEngine_SceneLoadModel(const char* pathUtf8);
RE_ENGINE_API int32_t RE_CALL ReEngine_SceneSetSkybox(const char* pathUtf8);

#ifdef __cplusplus
} // extern "C"
#endif