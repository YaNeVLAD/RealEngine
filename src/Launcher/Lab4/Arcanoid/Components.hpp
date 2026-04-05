#pragma once

#include <Runtime/Components.hpp>

#include "Constants.hpp"

namespace arcanoid
{

struct PaddleComponent
{
	float speed = constants::PaddleSpeed;
	float width = constants::PaddleWidth;
	float depth = constants::PaddleDepth;
	int moveDir = 0;
};

struct BallComponent
{
	re::Vector3f velocity = { 0.f, 0.f, 0.f };
	float radius = constants::BallRadius;
	float speed = constants::BallSpeed;
	bool isAttachedToPaddle = true;
};

struct BrickComponent
{
	int health = 1;
	float width = constants::BrickWidth;
	float depth = constants::BrickDepth;
	int scoreValue = constants::ScorePerBrick;
};

struct GameStateComponent
{
	int score = 0;
	int lives = constants::StartingLives;
	int currentLevel = 1;
	bool requestNextLevel = false;
};

} // namespace arkanoid