#include "InputManager.h"
#include "IConfigFile.h"
#include "IFileSystem.h"
#include <iostream>
#include <fstream>

InputManager::InputManager(const std::shared_ptr<IConfigFile>& config, const std::shared_ptr<IFileSystem>& fs)
	:Config(config), MyFileSystem(fs)
{
	KeyBindings[GameInputKey_Up] = Config->GetUIntValue("UpKey");
	KeyBindings[GameInputKey_Right] = Config->GetUIntValue("RightKey");
	KeyBindings[GameInputKey_Down] = Config->GetUIntValue("DownKey");
	KeyBindings[GameInputKey_Left] = Config->GetUIntValue("LeftKey");
	KeyBindings[GameInputKey_CrystalBall] = Config->GetUIntValue("CrystalBallKey");
    KeyBindings[GameInputKey_Back] = Config->GetUIntValue("BackKey");
    KeyBindings[GameInputKey_MapMakerChangeTool] = Config->GetUIntValue("MapMakerChangeToolKey");
    CurrentState.Up = false;
    CurrentState.Down = false;
    CurrentState.Left = false;
    CurrentState.Right = false;
    CurrentState.Back = false;
    CurrentState.Quit = false;
    CurrentState.MapMakerChangeTool = false;
}

GameInputState InputManager::PollEvents()
{
    
    switch (RecordingState)
    {
    case InputRecordingState::NotRecording:
        PollEventsNotRecording();
        break;
    case InputRecordingState::Recording:
        PollEventsRecording();
        break;
    case InputRecordingState::PlayingBack:
        PollEventsReplayingRecording();
        break;
    }
    return CurrentState;
}

void InputManager::SetRecordingState(InputRecordingState newState)
{

    switch (newState)
    {
    case InputRecordingState::Recording:
        if (RecordingState != InputRecordingState::Recording)
        {
            InputSnaps.clear();
            FrameCounter = 0;
        }
        break;
    case InputRecordingState::PlayingBack:
        if (RecordingState != InputRecordingState::PlayingBack)
        {
            NextSnapIndex = 0;
            FrameCounter = 0;
        }
        break;
    }
    RecordingState = newState;
}

void InputManager::PollEventsNotRecording()
{
    SDL_Event event;

    CurrentState.PreviousUp = CurrentState.Up;
    CurrentState.PreviousDown = CurrentState.Down;
    CurrentState.PreviousLeft = CurrentState.Left;
    CurrentState.PreviousRight = CurrentState.Right;
    CurrentState.PreviousCrystalBall = CurrentState.CrystalBall;
    CurrentState.PreviousBack = CurrentState.Back;
    CurrentState.PreviousMapMakerChangeTool = CurrentState.MapMakerChangeTool;

    /* Poll for events. SDL_PollEvent() returns 0 when there are no  */
    /* more events on the event queue, our while loop will exit when */
    /* that occurs.                                                  */
    while (SDL_PollEvent(&event)) {
        /* We are only worried about SDL_KEYDOWN and SDL_KEYUP events */
        switch (event.type) {
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
        {
            bool newStateToSet = event.type == SDL_EVENT_KEY_DOWN;
            SDL_Keycode sym = event.key.keysym.scancode;
            for (int i = 0; i < GameInputKey_NumKeys; i++)
            {
                if (sym == KeyBindings[i])
                {
                    switch ((GameInputKey)i)
                    {
                    case GameInputKey_Up:
                        CurrentState.Up = newStateToSet;
                        break;
                    case GameInputKey_Right:
                        CurrentState.Right = newStateToSet;
                        break;
                    case GameInputKey_Down:
                        CurrentState.Down = newStateToSet;
                        break;
                    case GameInputKey_Left:
                        CurrentState.Left = newStateToSet;
                        break;
                    case GameInputKey_CrystalBall:
                        CurrentState.CrystalBall = newStateToSet;
                        break;
                    case GameInputKey_Back:
                        CurrentState.Back = newStateToSet;
                        break;
                    case GameInputKey_MapMakerChangeTool:
                        CurrentState.MapMakerChangeTool = newStateToSet;
                        break;
                    }
                }
            }
        }
        break;
        case SDL_EVENT_QUIT:
            CurrentState.Quit = true;
            break;
        default:
            break;
        }
    }
}

void InputManager::PollEventsRecording()
{

    PollEventsNotRecording();
    if (CurrentState.AnyRecordingSignificantChange())
    {
        InputSnaps.push_back({ CurrentState, FrameCounter });
        //std::cout << "Snaps size: " << InputSnaps.size() << " Current state: ";// << std::to_string((int)CurrentState) << "Frame Counter: "<< FrameCounter << "\n";
        //CurrentState.PrintCurrent();
        //std::cout << "Frame Counter: " << FrameCounter << "\n";
    }
    ++FrameCounter;
}

void InputManager::PollEventsReplayingRecording()
{
    CurrentState.PreviousUp = CurrentState.Up;
    CurrentState.PreviousDown = CurrentState.Down;
    CurrentState.PreviousLeft = CurrentState.Left;
    CurrentState.PreviousRight = CurrentState.Right;
    CurrentState.PreviousCrystalBall = CurrentState.CrystalBall;
    CurrentState.PreviousBack = CurrentState.Back;
    CurrentState.PreviousMapMakerChangeTool = CurrentState.MapMakerChangeTool;

    if (FrameCounter++ == InputSnaps[NextSnapIndex].Frame)
    {
        CurrentState = InputSnaps[NextSnapIndex++].State;
        if (NextSnapIndex >= InputSnaps.size())
        {
            // event or something here
            NextSnapIndex = 0;
        }
    }
    
}

void InputManager::SaveRecordingFile(char* data, size_t size)
{
    std::ofstream wf(MyFileSystem->GetReplaysFolderPath() + ReplayFileName, std::ios::out | std::ios::binary);
    wf.write(data, size);
}

size_t InputManager::GetCurrentReplayRequiredBufferSize() const
{
    size_t bufferSize = InputSnaps.size() * sizeof(InputSnap) + sizeof(SavedRecordingHeader);
    return bufferSize;
}

void InputManager::WriteReplayBuffer(char* data, size_t size, u32 score) const
{
    SavedRecordingHeader header;
    header.Score = score;
    header.NumSnaps = InputSnaps.size();
    std::string name = Config->GetStringValue("PlayerName");
    strcpy_s(header.Name, name.c_str());
    memcpy(data, &header, sizeof(header));
    memcpy(data + sizeof(SavedRecordingHeader), InputSnaps.data(), sizeof(InputSnap) * header.NumSnaps);
}

void InputManager::LoadRecordingFile()
{
    LoadRecordingFile(MyFileSystem->GetReplaysFolderPath() + ReplayFileName);
}

void InputManager::LoadRecordingFile(const std::string& filePath)
{
    std::cout << "loading recording " << filePath << "\n";
    std::ifstream input(filePath, std::ios::binary);

    // copies all data into buffer
    std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(input), {});
    SavedRecordingHeader header;
    memcpy(&header, buffer.data(), sizeof(SavedRecordingHeader));
    InputSnap* snaps = (InputSnap*)(buffer.data() + sizeof(SavedRecordingHeader));
    InputSnaps.clear();
    for (int i = 0; i < header.NumSnaps; i++)
    {
        InputSnaps.push_back(snaps[i]);
    }
    InputSnaps[0].State.PreviousCrystalBall = false;
    InputSnaps[0].State.CrystalBall = false;
}

