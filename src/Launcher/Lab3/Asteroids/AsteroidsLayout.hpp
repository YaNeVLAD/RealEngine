#pragma once

#include <RenderCore/Assets/AssetManager.hpp>
#include <RenderCore/IWindow.hpp>
#include <Runtime/Components.hpp>
#include <Runtime/Layout.hpp>

#include "AsteroidsComponents.hpp"

class AsteroidsLayout final : public re::Layout
{
public:
	AsteroidsLayout(re::Application& app, re::render::IWindow& window);

	void OnCreate() override;
	void OnUpdate(re::core::TimeDelta dt) override;
	void OnEvent(re::Event const& event) override;

private:
	void StartGame();
	void SpawnAsteroid(int size, re::Vector2f pos, re::Vector2f vel);
	void SpawnParticle(re::Vector2f pos, re::Vector2f vel);
	void CreateShip();
	void CreateUI();
	void UpdateUI();
	void ShowGameOverDialog();

	re::render::IWindow& m_window;
	re::AssetManager m_assetManager;

	re::ecs::Entity m_gameStateEntity = re::ecs::Entity::INVALID_ID;
	re::ecs::Entity m_shipEntity = re::ecs::Entity::INVALID_ID;
	re::ecs::Entity m_scoreText = re::ecs::Entity::INVALID_ID;
	re::ecs::Entity m_livesText = re::ecs::Entity::INVALID_ID;

	bool m_gameOverDialogShown = false;
	int m_lastScore = -1;
	int m_lastLives = -1;
};