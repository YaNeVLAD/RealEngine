#pragma once

#include <Audio/Player.hpp>
#include <Audio/SineWaveGenerator.hpp>
#include <Core/Math/Vector3.hpp>
#include <RenderCore/Keyboard.hpp>

#include <atomic>
#include <cmath>
#include <memory>
#include <vector>

struct PianoKeyComponent
{
	re::Keyboard::Key key;
	int voiceIndex;
	bool isBlack;
};

class PianoVoice
{
public:
	re::audio::SineWaveGenerator osc;
	std::atomic<bool> isPressed{ false };

	float currentVolume = 0.0f;
	float releaseStep = 0.0f;

	PianoVoice(const ma_uint32 sampleRate, const float frequency)
		: osc(sampleRate, frequency, 1.0f)
	{
		releaseStep = 1.0f / (sampleRate * 0.5f);
	}

	float Process()
	{
		if (isPressed.load(std::memory_order_relaxed))
		{
			currentVolume = 1.0f;
		}
		else
		{
			currentVolume -= releaseStep;
			if (currentVolume < 0.0f)
			{
				currentVolume = 0.0f;
			}
		}

		if (currentVolume > 0.0f)
		{
			return osc.GetNextSample() * currentVolume;
		}

		return 0.0f;
	}
};

namespace KeyInitializer
{

struct KeyDef
{
	re::Keyboard::Key key;
	int halfSteps;
	bool isBlack;
	re::String label;
};

constexpr std::size_t KEYS_COUNT = 24;

struct PianoData
{
	std::vector<KeyDef> layout;
	std::vector<std::unique_ptr<PianoVoice>> voices;
	std::unique_ptr<re::audio::Player> player;
};

inline PianoData Prepare(ma_uint32 sampleRate = 48000)
{
	PianoData data;

	data.layout = std::vector<KeyDef>({
		{ re::Keyboard::Key::Z, -9, false, "Z" },
		{ re::Keyboard::Key::S, -8, true, "S" },
		{ re::Keyboard::Key::X, -7, false, "X" },
		{ re::Keyboard::Key::D, -6, true, "D" },
		{ re::Keyboard::Key::C, -5, false, "C" },
		{ re::Keyboard::Key::V, -4, false, "V" },
		{ re::Keyboard::Key::G, -3, true, "G" },
		{ re::Keyboard::Key::B, -2, false, "B" },
		{ re::Keyboard::Key::H, -1, true, "H" },
		{ re::Keyboard::Key::N, 0, false, "N" },
		{ re::Keyboard::Key::J, 1, true, "J" },
		{ re::Keyboard::Key::M, 2, false, "M" },
		{ re::Keyboard::Key::Q, 3, false, "Q" },
		{ re::Keyboard::Key::Num2, 4, true, "2" },
		{ re::Keyboard::Key::W, 5, false, "W" },
		{ re::Keyboard::Key::Num3, 6, true, "3" },
		{ re::Keyboard::Key::E, 7, false, "E" },
		{ re::Keyboard::Key::R, 8, false, "R" },
		{ re::Keyboard::Key::Num5, 9, true, "5" },
		{ re::Keyboard::Key::T, 10, false, "T" },
		{ re::Keyboard::Key::Num6, 11, true, "6" },
		{ re::Keyboard::Key::Y, 12, false, "Y" },
		{ re::Keyboard::Key::Num7, 13, true, "7" },
		{ re::Keyboard::Key::U, 14, false, "U" },
	});

	data.voices.reserve(KEYS_COUNT);

	for (const auto& def : data.layout)
	{
		const float frequency = 440.0f * std::pow(2.0f, def.halfSteps / 12.0f);
		data.voices.push_back(std::make_unique<PianoVoice>(sampleRate, frequency));
	}

	data.player = std::make_unique<re::audio::Player>(ma_format_f32, 1, sampleRate);

	return data;
}

} // namespace KeyInitializer