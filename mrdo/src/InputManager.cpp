#include "InputManager.h"
#include "IConfigFile.h"

InputManager::InputManager(const std::shared_ptr<IConfigFile>& config)
	:Config(config)
{
	KeyBindings[GameInputKey_Up] = Config->GetUIntValue("UpKey");
	KeyBindings[GameInputKey_Right] = Config->GetUIntValue("RightKey");
	KeyBindings[GameInputKey_Down] = Config->GetUIntValue("DownKey");
	KeyBindings[GameInputKey_Left] = Config->GetUIntValue("LeftKey");
	KeyBindings[GameInputKey_CrystalBall] = Config->GetUIntValue("CrystalBallKey");
    KeyBindings[GameInputKey_Back] = Config->GetUIntValue("BackKey");
    CurrentState.Up = false;
    CurrentState.Down = false;
    CurrentState.Left = false;
    CurrentState.Right = false;
    CurrentState.Back = false;
    CurrentState.Quit = false;
}

GameInputState InputManager::PollEvents()
{
    SDL_Event event;

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
    return CurrentState;
}

