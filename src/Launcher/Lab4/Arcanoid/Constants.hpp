#pragma once

#include <Core/Math/Vector3.hpp>

#include <numbers>

namespace arcanoid::constants
{

constexpr int StartingLives = 3;
constexpr int ScorePerBrick = 10;

constexpr float FieldLimitX = 10.0f;
constexpr float FieldTopZ = -15.0f;
constexpr float FieldBottomZ = 10.0f;

constexpr float PaddleSpeed = 20.0f;
constexpr float PaddleWidth = 4.0f;
constexpr float PaddleDepth = 1.0f;
constexpr float PaddleHeight = 1.0f;
constexpr re::Vector3f PaddleSpawnPos = { 0.f, 0.f, 7.f };
constexpr float PaddleBounceFactor = 0.8f;

constexpr re::Vector3f BallSpawnPos = { 0.f, 0.f, 6.f };
constexpr float BallSpeed = 20.0f;
constexpr float BallRadius = 0.5f;
constexpr float BallRotation = 360.f / (2.f * std::numbers::pi_v<float> * BallRadius);

constexpr float BrickWidth = 1.0f;
constexpr float BrickDepth = 1.0f;
constexpr float BrickHeight = 1.0f;

constexpr int BaseRows = 0;
constexpr int MaxCols = 16;
constexpr float LevelStartX = -9.0f;
constexpr float LevelStartZ = -10.0f;
constexpr float SpacingX = 1.2f;
constexpr float SpacingZ = 1.2f;

constexpr re::Vector3f CameraPos = { 0.f, 20, 16.5 };
constexpr re::Vector3f CameraRot = { -50, 270, 0 };
constexpr re::Vector3f LightPos = { 0.f, 3.f, 0.f };
constexpr re::Vector3f LightRot = { -45.f, -45.f, 0.f };

constexpr re::Vector3f BgPos = { 0.f, -2.f, -5.f };
constexpr re::Vector3f BgScale = { 25.f, 0.1f, 30.f };

} // namespace arcanoid::constants