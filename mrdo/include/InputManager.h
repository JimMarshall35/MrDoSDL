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
	GameInputKey_Back,
	GameInputKey_MapMakerChangeTool,
	GameInputKey_NumKeys
};

struct GameInputState
{
	u8 Up : 1;
	u8 Down : 1;
	u8 Left : 1;
	u8 Right : 1;
	u8 CrystalBall : 1;
	u8 Back : 1;
	u8 Quit : 1;
	u8 MapMakerChangeTool : 1;

	u8 PreviousUp : 1;
	u8 PreviousDown : 1;
	u8 PreviousLeft : 1;
	u8 PreviousRight : 1;
	u8 PreviousCrystalBall : 1;
	u8 PreviousBack : 1;
	u8 PreviousMapMakerChangeTool : 1;

	bool AnyDirectionPressed()
	{
		return Up || Down || Left || Right;
	}

	bool CrystalBallPress() const
	{
		return !PreviousCrystalBall && CrystalBall;
	}

	bool UpPress() const
	{
		return !PreviousUp && Up;
	}

	bool DownPress() const
	{
		return !PreviousDown && Down;
	}

	bool LeftPress() const
	{
		return !PreviousLeft && Left;
	}

	bool RightPress() const
	{
		return !PreviousRight && Right;
	}

	bool BackPress() const
	{
		return !PreviousBack && Back;
	}

	bool MapMakerChangeToolPress() const
	{
		return !PreviousMapMakerChangeTool && MapMakerChangeTool;
	}
};


class InputManager
{
public:
	InputManager(const std::shared_ptr<IConfigFile>& config);
	GameInputState PollEvents();
private:
	std::shared_ptr<IConfigFile> Config;
	SDL_Keycode KeyBindings[GameInputKey_NumKeys];
	GameInputState CurrentState;
};