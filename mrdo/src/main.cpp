//Using SDL and standard IO
#include <SDL.h>
#include <stdio.h>
#include "BackgroundTileAssetManager.h"
#include "AnimationAssetManager.h"
#include "FileSystem.h"
#include "ConfigFile.h"
#include "TiledWorld.h"
#include "InputManager.h"
#include "Character.h"
#include <iostream>
#include "GameFramework.h"
#include "GameLayer.h"
#include "FontAssetManager.h"
#include "TextRenderer.h"
#include "FrontEndLayer.h"
#include "MapMakerLevelSelectLayer.h"
#include "MapMakerCreateNewLevelDialogue.h"
#include "MapMakerLevelSelectedDialogue.h"
#include "MapMakerLayer.h"
#include "PathFinding.h"


int main(int argc, char* args[])
{
    char* exePath = args[0];
    //The window we'll be rendering to
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;

    //The surface contained by the window
    SDL_Surface* screenSurface = NULL;

    //Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    }
    else
    {
        // just hard code this for now - all levels are this size
        uvec2 windowSize = { 12 * 16, 15 * 16 }; //level->GetRequiredBaseWindowSize();
        float scaleFactor = 3.0f;
        const int SCREEN_WIDTH = windowSize.x * scaleFactor;
        const int SCREEN_HEIGHT = windowSize.y * scaleFactor;

        //Create window
        SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &window, &renderer);
        SDL_SetRenderScale(renderer, scaleFactor, scaleFactor);
        if (window == NULL)
        {
            printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        }
        else
        {
            //Get window surface
            screenSurface = SDL_GetWindowSurface(window);

            // Game systems setup
            // my rationale for making these shared ptrs is that they might be used in destructors of classes that depend on them
            // so I don't want the order of instantiation to matter as it would if they were stack allocated here and passed as raw ptrs for example

            std::shared_ptr<IFileSystem> fileSystem = std::make_shared<FileSystem>(exePath);
            std::shared_ptr<IConfigFile> configFile = std::make_shared<ConfigFile>(fileSystem);
            std::shared_ptr<IFontAssetManager> fontAssetManager = std::make_shared<FontAssetManager>(configFile, screenSurface);
            std::shared_ptr<TextRenderer> textRenderer = std::make_shared<TextRenderer>(fontAssetManager);
            std::shared_ptr<BackgroundTileAssetManager> backgroundTileAssetManager = std::make_shared<BackgroundTileAssetManager>(configFile);
            InputManager inputManager(configFile);

            std::shared_ptr<IAnimationAssetManager> animationAssetManager = std::make_shared<AnimationAssetManager>(configFile, screenSurface);
            
            // game framework layers
            Game game(fileSystem, configFile, backgroundTileAssetManager, animationAssetManager, textRenderer);
            FrontEndLayer frontend(textRenderer, SCREEN_WIDTH, SCREEN_HEIGHT);
            MapMakerLevelSelectLayer mapMakerLevelSelect(textRenderer, configFile, SCREEN_WIDTH, SCREEN_HEIGHT);
            MapMakerCreateNewLevelDialogue mapMakerCreateNewLevelDialogue(textRenderer, configFile, SCREEN_WIDTH, SCREEN_HEIGHT);
            MapMakerLevelSelectedDialogue mapMakerLevelSelectedDialogue(configFile, textRenderer, SCREEN_WIDTH, SCREEN_HEIGHT);
            MapMakerLayer MapMakerLayer(configFile, backgroundTileAssetManager, animationAssetManager, textRenderer);

            GameFramework::PushLayers("FrontEnd", GameLayerType::Draw | GameLayerType::Input | GameLayerType::Update, 0);

            PathFinding::Initialise(configFile->GetUIntValue("PathFindingPriorityQueueSize"));

            bool quit = false; 
            u32 simulationTime = 0;
            u32 realTime = 0;
            while (quit == false) 
            {
                realTime = SDL_GetTicks();
                while (simulationTime < realTime) {
                    simulationTime += 16; //Timeslice is ALWAYS 16ms (60 FPS). 
                    GameInputState state = inputManager.PollEvents();
                    quit = state.Quit;
                    GameFramework::RecieveInput(state);
                    GameFramework::Update(16);
                }

                SDL_FillSurfaceRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0, 0, 0));
                GameFramework::Draw(screenSurface, scaleFactor);
                GameFramework::EndFrame();
                SDL_UpdateWindowSurface(window);
            }

            PathFinding::DeInit();
        }
    }
    //Destroy window
    SDL_DestroyWindow(window);

    //Quit SDL subsystems
    SDL_Quit();


    return 0;
}