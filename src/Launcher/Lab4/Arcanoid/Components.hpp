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
	float paddleSizeTimer = 0.0f;
	float ballSpeedTimer = 0.0f;
};

enum class BonusType
{
	ExtraLife,
	Expand,
	Shrink,
	Slow,
	Fast,
	MultiBall
};

struct BonusComponent
{
	BonusType type;
};

} // namespace arcanoid