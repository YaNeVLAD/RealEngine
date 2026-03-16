#pragma once

#include <Core/Math/Vector2.hpp>
#include <ECS/Entity/Entity.hpp>

#include <vector>

struct VelocityComponent
{
	re::Vector2f linear = { 0.f, 0.f };
	float angular = 0.f;
};

struct AsteroidComponent
{
	int size = 3;
	std::vector<re::Vector2f> localPoints;
};

struct ShipComponent
{
	float fireCooldown = 0.f;
	float respawnTimer = 0.f;
	float invulnerabilityTimer = 0.f;
	re::ecs::Entity flameEntity = re::ecs::Entity::INVALID_ID;
};

struct BulletComponent
{
	float lifetime = 0.5f;
};

struct ParticleComponent
{
	float lifetime = 1.0f;
};

struct ScreenWrapComponent
{
	float radius = 0.f;
};

struct AsteroidGameStateComponent
{
	int score = 0;
	int lives = 3;
	bool isGameOver = false;
};

struct AsteroidButtonComponent
{
	enum class Action
	{
		PlayAgain,
		Quit
	} action;
};