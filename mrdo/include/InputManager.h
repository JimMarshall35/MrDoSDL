#pragma once
#include <memory>
#include "CommonTypedefs.h"
#include "SDL.h"
#include <vector>
#include <string>

class IFileSystem;
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

	bool CrystalBallRelease() const
	{
		return PreviousCrystalBall && !CrystalBall;
	}

	bool UpPress() const
	{
		return !PreviousUp && Up;
	}

	bool UpRelease() const
	{
		return PreviousUp && !Up;
	}

	bool DownPress() const
	{
		return !PreviousDown && Down;
	}

	bool DownRelease() const
	{
		return PreviousDown && !Down;
	}

	bool LeftPress() const
	{
		return !PreviousLeft && Left;
	}

	bool LeftRelease() const
	{
		return PreviousLeft && !Left;
	}

	bool RightPress() const
	{
		return !PreviousRight && Right;
	}

	bool RightRelease() const
	{
		return PreviousRight && !Right;
	}

	bool BackPress() const
	{
		return !PreviousBack && Back;
	}

	bool MapMakerChangeToolPress() const
	{
		return !PreviousMapMakerChangeTool && MapMakerChangeTool;
	}

	void PrintCurrent() const
	{
		printf("up: %i down: %i left: %i right: %i attack: %i", Up, Down, Left, Right, CrystalBall);
	}

	bool AnyRecordingSignificantChange()
	{
		return
			UpPress() || UpRelease() ||
			DownPress() || DownRelease() ||
			LeftPress() || LeftRelease() ||
			RightPress() || RightRelease() ||
			CrystalBallPress() || CrystalBallRelease();
	}
};

enum class InputRecordingState
{
	Recording,
	PlayingBack,
	NotRecording
};

struct InputSnap
{
	GameInputState State;
	u64 Frame;
};

#define RecordingFileNameMaxLen 32
struct SavedRecordingHeader
{
	int Version = 0;
	size_t NumSnaps = 0;
	char Name[RecordingFileNameMaxLen];
	u64 Reserved;
	u64 Reserved1;
};

class InputManager
{
public:
	InputManager(const std::shared_ptr<IConfigFile>& config, const std::shared_ptr<IFileSystem>& fs);
	GameInputState PollEvents();
	void SetRecordingState(InputRecordingState newState);
	InputRecordingState GetRecordingState();
	void ResetFrameCounter() { FrameCounter = 0; }
	void PollEventsNotRecording();
	void PollEventsRecording();
	void PollEventsReplayingRecording();
	void SaveRecordingFile();
	void LoadRecordingFile();
	void LoadRecordingFile(const std::string& fileName);

private:
	u64 FrameCounter = 0;
	u32 NextSnapIndex = 0;
	std::vector<InputSnap> InputSnaps;
	std::shared_ptr<IConfigFile> Config;
	SDL_Keycode KeyBindings[GameInputKey_NumKeys];
	GameInputState CurrentState;
	InputRecordingState RecordingState = InputRecordingState::NotRecording;
	const std::shared_ptr<IFileSystem> MyFileSystem;
	std::string ReplayFileName = "test.replay";
};