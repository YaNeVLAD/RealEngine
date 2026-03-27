#include <Audio/Player.hpp>

namespace re::audio
{

Player::Player(ma_format format, ma_uint32 channels, ma_uint32 sampleRate)
	: m_device(CreateConfig(format, channels, sampleRate))
{
	m_device.SetDataCallback([this](void* output, const void*, const ma_uint32 frameCount) {
		if (m_dataCallback)
		{
			m_dataCallback(output, frameCount);
		}
	});
}

void Player::Start()
{
	m_device.Start();
}

void Player::Stop()
{
	m_device.Stop();
}

ma_format Player::GetFormat() const noexcept
{
	return m_device->playback.format;
}

ma_uint32 Player::GetChannels() const noexcept
{
	return m_device->playback.channels;
}

ma_uint32 Player::GetSampleRate() const noexcept
{
	return m_device->sampleRate;
}

void Player::SetDataCallback(DataCallback callback)
{
	m_dataCallback = std::move(callback);
}

ma_device_config Player::CreateConfig(const ma_format format, const ma_uint32 channels, const ma_uint32 sampleRate)
{
	auto config = ma_device_config_init(ma_device_type_playback);
	auto& playback = config.playback;

	playback.format = format;
	playback.channels = channels;
	config.sampleRate = sampleRate;

	return config;
}

} // namespace re::audio