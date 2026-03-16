#pragma once

namespace asteroids
{

constexpr float Z_Asteroid = 5.f;
constexpr float Z_ShipFlame = 9.f;
constexpr float Z_Ship = 10.f;
constexpr float Z_Particle = 15.f;
constexpr float Z_UI_Background = 90.f;
constexpr float Z_UI_Text = 100.f;

constexpr float ShipAngularAcceleration = 1000.f;
constexpr float ShipLinearAcceleration = 600.f;
constexpr float ShipLinearFriction = 1.5f;
constexpr float ShipAngularFriction = 3.0f;
constexpr float ShipHitRadius = 15.0f;
constexpr float ShipFireCooldown = 0.25f;
constexpr float ShipRespawnTimer = 2.0f;
constexpr float ShipInvulnerabilityTimer = 3.0f;
constexpr float ShipBulletOffset = 25.f;

constexpr int AsteroidMaxSize = 3;
constexpr int AsteroidMaxCount = 5;
constexpr float AsteroidBaseRadius = 25.0f;
constexpr int AsteroidBaseSegments = 8;
constexpr int AsteroidScoreMultiplier = 100;

constexpr float BulletSpeed = 1000.f;
constexpr float BulletLifetime = 0.5f;
constexpr float BulletWidth = 5.f;
constexpr float BulletHeight = 10.f;

constexpr float ParticleLifetime = 1.0f;
constexpr int ParticleExplosionCount = 8;

} // namespace asteroids