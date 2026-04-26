#pragma once

#include <Core/Math/Vector3.hpp>
#include <RenderCore/StaticMesh.hpp>
#include <RenderCore/Texture.hpp>

#include <memory>

namespace battle_city
{

constexpr float BLOCK_SIZE = 3.0f;
constexpr float TANK_SIZE = 1.15f;
constexpr float PROJECTILE_RADIUS = 0.25f;
constexpr float PLAYER_SPEED = 8.0f;
constexpr float ENEMY_SPEED = 6.0f;
constexpr float PROJECTILE_SPEED = 20.0f;

enum class TeamType
{
	Player,
	Enemy
};

enum class BlockType
{
	Brick,
	Armor,
	Water,
	Ice,
	Tree
};

struct BlockComponent
{
	BlockType type{};
	int health = 1;
};

struct TankComponent
{
	TeamType team;
	float speed = 5.0f;
	float fireCooldown = 1.0f;
	float currentCooldown = 0.0f;
	re::Vector3f facingDir = { 0.f, 0.f, -1.f };
	bool onIce = false;
};

struct PlayerInputComponent
{
	bool up = false;
	bool down = false;
	bool left = false;
	bool right = false;
	bool shoot = false;
};

struct EnemyAiComponent
{
	float directionTimer = 0.0f;
	float fireTimer = 0.0f;
	re::Vector3f currentDir = { 0.f, 0.f, 1.f };
};

struct ProjectileComponent
{
	TeamType team{};
	float speed = 15.0f;
};

struct BaseComponent
{
	int health = 1;
};

struct EnemySpawnPointComponent
{
	bool dummy = false;
};

struct PlayerSpawnPointComponent
{
	bool dummy = false;
};

struct TurretComponent
{
	bool dummy = false;
};

struct GameStateComponent
{
	int lives = 3;
	int score = 0;
	int enemiesToSpawn = 20;
	int enemiesActive = 0;
	float spawnTimer = 2.0f;
	bool isGameOver = false;
	bool isVictory = false;

	re::Vector3f playerSpawnPos = re::Vector3f(0.f);
};

struct GameAssetsComponent
{
	std::shared_ptr<re::StaticMesh> cubeMesh;
	std::shared_ptr<re::StaticMesh> sphereMesh;

	std::shared_ptr<re::Model> playerModel;
	std::shared_ptr<re::Model> enemyModel;

	std::shared_ptr<re::Texture> projectileTex;
	std::shared_ptr<re::Texture> baseTex;
};

} // namespace battle_city