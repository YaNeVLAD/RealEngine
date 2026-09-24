#pragma once

#include <Engine/Export.hpp>

#include <stdint.h> // NOLINT(*-deprecated-headers)

#ifdef __cplusplus
extern "C" {
#endif

#define RE_ENGINE_API_VERSION 1u

typedef void(RE_CALL* ReEngine_LogCallback)(int32_t level, const char* messageUtf8, void* userdata);

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

#pragma pack(push, 8)

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

RE_ENGINE_API int RE_CALL ReEngine_Input_IsKeyPressed(int keyCode);
RE_ENGINE_API int RE_CALL ReEngine_Input_IsMouseButtonDown(int button);

RE_ENGINE_API uint64_t RE_CALL ReEngine_Viewport_Create(uint64_t hwnd, uint32_t width, uint32_t height);
RE_ENGINE_API int32_t RE_CALL ReEngine_Viewport_Render(uint64_t viewport);
RE_ENGINE_API int32_t RE_CALL ReEngine_Viewport_Resize(uint64_t viewport, uint32_t width, uint32_t height);
RE_ENGINE_API void RE_CALL ReEngine_Viewport_Destroy(uint64_t viewport);
RE_ENGINE_API int32_t RE_CALL ReEngine_Viewport_SetClearColor(uint64_t viewport, uint32_t rgba);

RE_ENGINE_API uint64_t RE_CALL ReEngine_Scene_CreateEntity(void);
RE_ENGINE_API int32_t RE_CALL ReEngine_Scene_IsEntityValid(uint64_t entity);
RE_ENGINE_API void RE_CALL ReEngine_Scene_DestroyEntity(uint64_t entity);
RE_ENGINE_API uint32_t RE_CALL ReEngine_Scene_GetEntityCount(void);
RE_ENGINE_API int32_t RE_CALL ReEngine_Scene_GetEntities(uint64_t* outIds, uint32_t capacity, uint32_t* outTotal);
RE_ENGINE_API uint32_t RE_CALL ReEngine_Scene_ClearEntities(void);
RE_ENGINE_API uint64_t RE_CALL ReEngine_Scene_SpawnPrimitive(int32_t kind, uint32_t rgba);
RE_ENGINE_API int32_t RE_CALL ReEngine_Scene_SetSimulating(int32_t simulating);
RE_ENGINE_API int32_t RE_CALL ReEngine_Scene_IsSimulating(void);
RE_ENGINE_API void RE_CALL ReEngine_Scene_ConfirmChanges(void);

RE_ENGINE_API uint64_t RE_CALL ReEngine_Entity_GetComponentMask(uint64_t entity);
RE_ENGINE_API int32_t RE_CALL ReEngine_Entity_AddComponent(uint64_t entity, int32_t component);
RE_ENGINE_API int32_t RE_CALL ReEngine_Entity_RemoveComponent(uint64_t entity, int32_t component);
RE_ENGINE_API int32_t RE_CALL ReEngine_Entity_GetFieldData(uint64_t entity, int32_t component, int32_t fieldIndex, void* outData, uint32_t maxBytes, uint32_t* outBytes);
RE_ENGINE_API int32_t RE_CALL ReEngine_Entity_SetFieldData(uint64_t entity, int32_t component, int32_t fieldIndex, const void* data, uint32_t bytes);
RE_ENGINE_API int32_t RE_CALL ReEngine_Entity_GetName(uint64_t entity, char* outName, uint32_t capacity, uint32_t* outNeeded);
RE_ENGINE_API int32_t RE_CALL ReEngine_Entity_SetName(uint64_t entity, const char* nameUtf8);

RE_ENGINE_API int32_t RE_CALL ReEngine_Component_GetFieldCount(int32_t component);
RE_ENGINE_API int32_t RE_CALL ReEngine_Component_GetFieldInfo(int32_t component, int32_t fieldIndex, ReEngine_ComponentFieldInfo* outInfo);

RE_ENGINE_API uint32_t RE_CALL ReEngine_Scene_LoadModel(const char* pathUtf8);
RE_ENGINE_API int32_t RE_CALL ReEngine_Scene_SetSkybox(const char* pathUtf8);

// Parent of the entity via HierarchyComponent, or 0xFFFFFFFF when none/invalid.
RE_ENGINE_API uint64_t RE_CALL ReEngine_Entity_GetParent(uint64_t entity);
// Direct children ids (up to capacity), total always reported in outTotal.
RE_ENGINE_API int32_t RE_CALL ReEngine_Entity_GetChildren(uint64_t entity, uint64_t* outIds, uint32_t capacity, uint32_t* outTotal);

#ifdef __cplusplus
} // extern "C"
#endif