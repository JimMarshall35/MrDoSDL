#pragma once
#include <memory>
#include "CommonTypedefs.h"
#include "SDL.h"

class IConfigFile;

enum GameInputKey
{
	GameInputKey_Up = 0,
	GameInputKey_Right,
	GameInputKey_Down,
	GameInputKey_Left,
	GameInputKey_CrystalBall,
	GameInputKey_NumKeys
};

struct GameInputState
{
	u8 Up : 1;
	u8 Down : 1;
	u8 Left : 1;
	u8 Right : 1;
	u8 CrystalBall : 1;
	u8 Quit : 1;
	bool AnyDirectionPressed()
	{
		return Up || Down || Left || Right;
	}
};


class InputManager
{
public:
	InputManager(const std::shared_ptr<IConfigFile>& config);
	GameInputState PollEvents();
	void HandleKeyEvent(const SDL_Event& event);
private:
	std::shared_ptr<IConfigFile> Config;
	SDL_Keycode KeyBindings[GameInputKey_NumKeys];
	GameInputState CurrentState;
};