#pragma once

#include <Audio/Export.hpp>

#include <miniaudio.h>

#include <numbers>

namespace re::audio
{

class RE_AUDIO_API SineWaveGenerator
{
public:
	SineWaveGenerator(ma_uint32 sampleRate, ma_float frequency, ma_float amplitude = 1.f, ma_float startPhase = 0.f);

	ma_float GetNextSample();

private:
	ma_uint32 m_sampleRate;
	ma_float m_frequency;
	ma_float m_amplitude;
	ma_float m_phase;
	ma_float m_phaseShift = static_cast<ma_float>(2.f * std::numbers::pi * m_frequency / m_sampleRate);
};

} // namespace re::audio
