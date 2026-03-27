#include <Audio/SineWaveGenerator.hpp>

#include <cmath>

namespace re::audio
{

SineWaveGenerator::SineWaveGenerator(const ma_uint32 sampleRate, const ma_float frequency, const ma_float amplitude, const ma_float startPhase)
	: m_sampleRate{ sampleRate }
	, m_frequency{ frequency }
	, m_amplitude{ amplitude }
	, m_phase{ startPhase }
{
}

ma_float SineWaveGenerator::GetNextSample()
{
	constexpr auto twoPi = static_cast<ma_float>(2.f * std::numbers::pi);

	const auto sample = m_amplitude * std::sin(m_phase);
	m_phase = std::fmod(m_phase + m_phaseShift, twoPi);

	return sample;
}

} // namespace re::audio