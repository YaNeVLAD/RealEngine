#pragma once

#include <ECS/Scene.hpp>
#include <ECS/System/System.hpp>
#include <Runtime/Components.hpp>

#include "AsteroidsComponents.hpp"
#include "AsteroidsConstants.hpp"

#include <functional>

inline bool PointInTriangle(const re::Vector2f p, const re::Vector2f a, const re::Vector2f b, const re::Vector2f c)
{
	auto sign = [](const re::Vector2f p1, const re::Vector2f p2, const re::Vector2f p3) {
		return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
	};
	const float d1 = sign(p, a, b);
	const float d2 = sign(p, b, c);
	const float d3 = sign(p, c, a);
	const bool hasNeg = (d1 < 0.0f) || (d2 < 0.0f) || (d3 < 0.0f);
	const bool hasPos = (d1 > 0.0f) || (d2 > 0.0f) || (d3 > 0.0f);
	return !(hasNeg && hasPos);
}

class AsteroidCollisionSystem final : public re::ecs::System
{
public:
	using SpawnAsteroidCallback = std::function<void(int, re::Vector2f, re::Vector2f)>;
	using SpawnParticleCallback = std::function<void(re::Vector2f, re::Vector2f)>;

	AsteroidCollisionSystem(SpawnAsteroidCallback spawnAst, SpawnParticleCallback spawnPart)
		: m_spawnAsteroid(std::move(spawnAst))
		, m_spawnParticle(std::move(spawnPart))
	{
	}

	void Update(re::ecs::Scene& scene, re::core::TimeDelta dt) override
	{
		AsteroidGameStateComponent* gameState = nullptr;
		for (auto&& [entity, state] : *scene.CreateView<AsteroidGameStateComponent>())
		{
			gameState = &state;
			break;
		}

		if (gameState && gameState->isGameOver)
		{
			return;
		}

		HandleBulletAsteroidCollisions(scene, gameState);

		HandleShipAsteroidCollisions(scene, gameState);
	}

private:
	void HandleBulletAsteroidCollisions(re::ecs::Scene& scene, AsteroidGameStateComponent* gameState) const
	{
		using namespace asteroids;

		for (auto&& [bEnt, bTrans, bullet] : *scene.CreateView<re::TransformComponent, BulletComponent>())
		{
			for (auto&& [aEnt, aTrans, ast] : *scene.CreateView<re::TransformComponent, AsteroidComponent>())
			{
				if (CheckPointInAsteroid({ bTrans.position.x, bTrans.position.y }, aTrans, ast))
				{
					if (gameState)
					{
						gameState->score += AsteroidScoreMultiplier * (4 - ast.size);
					}

					SplitAsteroid(ast.size, { aTrans.position.x, aTrans.position.y });

					scene.DestroyEntity(aEnt);
					scene.DestroyEntity(bEnt);
					break;
				}
			}
		}
	}

	void HandleShipAsteroidCollisions(re::ecs::Scene& scene, AsteroidGameStateComponent* gameState) const
	{
		using namespace asteroids;

		for (auto&& [sEnt, sTrans, ship] : *scene.CreateView<re::TransformComponent, ShipComponent>())
		{
			if (ship.invulnerabilityTimer > 0.f || ship.respawnTimer > 0.f)
			{
				continue;
			}

			for (auto&& [aEnt, aTrans, ast] : *scene.CreateView<re::TransformComponent, AsteroidComponent>())
			{
				const re::Vector2f sPos2D = { sTrans.position.x, sTrans.position.y };
				const re::Vector2f aPos2D = { aTrans.position.x, aTrans.position.y };
				const float dist = (sPos2D - aPos2D).Length();

				if (const float hitRadius = ast.size * AsteroidBaseRadius + ShipHitRadius; dist < hitRadius)
				{
					ship.invulnerabilityTimer = 0.0f;
					ship.respawnTimer = ShipRespawnTimer;
					sTrans.scale = { 0.f, 0.f, 0.f };

					if (gameState)
					{
						gameState->lives--;
						if (gameState->lives <= 0)
						{
							gameState->isGameOver = true;
						}
					}

					for (int k = 0; k < ParticleExplosionCount; ++k)
					{
						const re::Vector2f pVel = { (rand() % 600 - 300) * 1.f, (rand() % 600 - 300) * 1.f };
						m_spawnParticle({ sTrans.position.x, sTrans.position.y }, pVel);
					}

					SplitAsteroid(ast.size, { aTrans.position.x, aTrans.position.y });
					scene.DestroyEntity(aEnt);
					break;
				}
			}
		}
	}

	static bool CheckPointInAsteroid(const re::Vector2f& point, const re::TransformComponent& aTrans, const AsteroidComponent& ast)
	{
		for (std::size_t i = 0; i < ast.localPoints.size(); ++i)
		{
			re::Vector2f p1 = ast.localPoints[i];
			re::Vector2f p2 = ast.localPoints[(i + 1) % ast.localPoints.size()];

			const re::Vector2f A = { aTrans.position.x, aTrans.position.y };
			const re::Vector2f B = A + p1.Rotate(aTrans.rotation.z);
			const re::Vector2f C = A + p2.Rotate(aTrans.rotation.z);

			if (PointInTriangle(point, A, B, C))
			{
				return true;
			}
		}
		return false;
	}

	void SplitAsteroid(const int currentSize, const re::Vector2f& position) const
	{
		using namespace asteroids;

		if (currentSize > 1)
		{
			const int newSize = currentSize - 1;
			for (int k = 0; k < 2; ++k)
			{
				const re::Vector2f vel = { (rand() % 400 - 200) * 1.f, (rand() % 400 - 200) * 1.f };
				m_spawnAsteroid(newSize, position, vel);
			}
		}
	}

	SpawnAsteroidCallback m_spawnAsteroid;
	SpawnParticleCallback m_spawnParticle;
};