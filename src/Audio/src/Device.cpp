#include <Audio/Device.hpp>

#include <Audio/ErrorCategory.hpp>

#include <span>
#include <stdexcept>

namespace re::audio
{

Device::Device(ma_device_config config)
{
	config.pUserData = this;
	config.dataCallback = [](ma_device* device, void* output, const void* input, const ma_uint32 frameCount) {
		static_cast<Device*>(device->pUserData)->OnDataCallback(output, input, frameCount);
	};

	if (ma_device_init(nullptr, &config, &m_device) != MA_SUCCESS)
	{
		throw std::runtime_error("Device initialization failed");
	}
}

Device::~Device()
{
	ma_device_uninit(&m_device);
}

void Device::Start()
{
	if (const auto result = ma_device_start(&m_device); result != MA_SUCCESS)
	{
		throw std::system_error(result, ErrorCategory());
	}
}

void Device::Stop()
{
	if (const auto result = ma_device_stop(&m_device); result != MA_SUCCESS)
	{
		throw std::system_error(result, ErrorCategory());
	}
}

ma_device* Device::operator->() noexcept
{
	return &m_device;
}

const ma_device* Device::operator->() const noexcept
{
	return &m_device;
}

void Device::SetDataCallback(DataCallback dataCallback)
{
	m_dataCallback = std::move(dataCallback);
}

void Device::OnDataCallback(void* output, const void* input, const ma_uint32 frameCount) const
{
	if (m_dataCallback)
	{
		m_dataCallback(output, input, frameCount);
	}
}

} // namespace re::audio