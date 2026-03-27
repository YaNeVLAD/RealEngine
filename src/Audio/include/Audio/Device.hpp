#pragma once

#include <miniaudio.h>

#include <functional>

namespace re::audio
{

class Device
{
public:
	using DataCallback = std::function<void(void*, const void*, ma_uint32)>;

	explicit Device(ma_device_config config);

	Device(const Device&) = delete;
	Device& operator=(const Device&) = delete;

	~Device();

	void Start();

	void Stop();

	ma_device* operator->() noexcept;

	const ma_device* operator->() const noexcept;

	void SetDataCallback(DataCallback dataCallback);

private:
	void OnDataCallback(void* output, const void* input, ma_uint32 frameCount) const;

	ma_device m_device{};
	DataCallback m_dataCallback;
};

} // namespace re::audio