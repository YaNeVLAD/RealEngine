#include <Engine/EngineContext.hpp>

#include <RenderCore/Keyboard.hpp>
#include <RenderCore/Mouse.hpp>

int ReEngine_Input_IsKeyPressed(int keyCode)
{
	return re::Keyboard::IsKeyPressed(static_cast<re::Keyboard::Key>(keyCode));
}

int ReEngine_Input_IsMouseButtonDown(int button)
{
	return re::Mouse::IsButtonPressed(static_cast<re::Mouse::Button>(button));
}