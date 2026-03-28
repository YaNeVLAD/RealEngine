#pragma once

#include <Audio/Player.hpp>
#include <Core/Math/Color.hpp>
#include <Core/Math/Vector2.hpp>
#include <Core/Math/Vector3.hpp>
#include <ECS/Scene/Scene.hpp>
#include <RenderCore/Keyboard.hpp>
#include <Runtime/Application.hpp>
#include <Runtime/Components.hpp>

#include "PianoInitializer.hpp"

#include <memory>
#include <vector>

struct PianoLayout final : re::Layout
{
	PianoLayout(re::Application& app, re::render::IWindow& window)
		: Layout(app)
		, m_window(window)
	{
	}

	void OnCreate() override
	{
		auto& scene = GetScene();
		m_piano = KeyInitializer::Prepare();
		m_piano.player->SetDataCallback([this](void* output, const ma_uint32 frameCount) {
			constexpr float PRE_GAIN = 0.5f;
			constexpr float MASTER_GAIN = 0.8f;
			// Нужно сбрасывать фазу, потому что сейчас при повторном нажатии клавиши будет щелчок

			const auto fOutput = static_cast<float*>(output);
			for (ma_uint32 i = 0; i < frameCount; ++i)
			{
				float mixedSample = 0.0f;
				for (const auto& voice : m_piano.voices)
				{
					mixedSample += voice->GetNextSample();
				}
				fOutput[i] = std::tanh(mixedSample * PRE_GAIN) * MASTER_GAIN;
			}
		});

		constexpr float WHITE_WIDTH = 50.0f;
		constexpr float BLACK_WIDTH = 30.0f;
		float currentWhiteX = -350.0f;

		for (std::size_t i = 0; i < m_piano.layout.size(); ++i)
		{
			const auto& def = m_piano.layout[i];

			re::Vector3f pos;
			re::Vector2f size;

			if (def.isBlack)
			{
				pos = { currentWhiteX - WHITE_WIDTH / 2.f, -40.f, 1.f };
				size = { BLACK_WIDTH, 120.f };
			}
			else
			{
				pos = { currentWhiteX, 0.f, 0.f };
				size = { WHITE_WIDTH - 2.f, 200.f };
				currentWhiteX += WHITE_WIDTH;
			}

			scene.CreateEntity()
				.Add<PianoKeyComponent>({ def.key, static_cast<int>(i), def.isBlack })
				.Add<re::TransformComponent>({ .position = pos })
				.Add<re::RectangleComponent>({
					.color = def.isBlack ? re::Color::Black : re::Color::White,
					.size = size,
				})
				.Add<re::TextComponent>({
					.text = def.label,
					.color = def.isBlack ? re::Color::White : re::Color::Black,
					.size = 14.f,
				});
		}
	}

	void OnAttach() override
	{
		m_window.SetBackgroundColor(re::Color{ 40, 40, 40 });
		if (m_piano.player)
		{
			m_piano.player->Start();
		}
	}

	void OnDetach() override
	{
		if (m_piano.player)
		{
			m_piano.player->Stop();
		}
	}

	void OnUpdate(const re::core::TimeDelta deltaTime) override
	{
		const auto mousePos = m_window.ToWorldPos(re::Mouse::GetPosition());
		const bool isMouseLeftPressed = re::Mouse::IsButtonPressed(re::Mouse::Button::Left);

		int hoveredVoiceIndex = -1;
		const auto view = GetScene().CreateView<PianoKeyComponent, re::RectangleComponent, re::TransformComponent>();
		for (auto&& [entity, pianoKey, rect, transform] : *view)
		{
			if (!pianoKey.isBlack)
			{
				continue;
			}

			if (IsPointInRect(mousePos, transform.position, rect.size))
			{
				hoveredVoiceIndex = pianoKey.voiceIndex;
				break;
			}
		}

		if (hoveredVoiceIndex == -1)
		{
			for (auto&& [entity, pianoKey, rect, transform] : *view)
			{
				if (pianoKey.isBlack)
				{
					continue;
				}

				if (IsPointInRect(mousePos, transform.position, rect.size))
				{
					hoveredVoiceIndex = pianoKey.voiceIndex;
					break;
				}
			}
		}

		for (auto&& [entity, pianoKey, rect, transform] : *view)
		{
			const bool isKeyboardPressed = re::Keyboard::IsKeyPressed(pianoKey.key);
			const bool isMousePressedOnThisKey = isMouseLeftPressed && (pianoKey.voiceIndex == hoveredVoiceIndex);

			const bool isPressed = isKeyboardPressed || isMousePressedOnThisKey;

			m_piano.voices[pianoKey.voiceIndex]->SetPressed(isPressed);

			if (isPressed)
			{
				rect.color = pianoKey.isBlack ? PRESSED_BLACK_KEY : PRESSED_WHITE_KEY;
			}
			else
			{
				rect.color = pianoKey.isBlack ? re::Color::Black : re::Color::White;
			}
		}
	}

private:
	static bool IsPointInRect(const re::Vector2f& point, const re::Vector3f& pos, const re::Vector2f& size)
	{
		const float dx = std::abs(point.x - pos.x);
		const float dy = std::abs(point.y - pos.y);

		return dx <= size.x * 0.5f && dy <= size.y * 0.5f;
	}

private:
	static constexpr re::Color PRESSED_BLACK_KEY = { 100, 100, 100 };
	static constexpr re::Color PRESSED_WHITE_KEY = { 200, 200, 200 };

	re::render::IWindow& m_window;

	KeyInitializer::PianoData m_piano{};
};