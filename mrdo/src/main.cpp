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
//Screen dimension constants


int main(int argc, char* args[])
{
    char* exePath = args[0];
    //The window we'll be rendering to
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;

    //The surface contained by the window
    SDL_Surface* screenSurface = NULL;


    // Game systems setup
    std::shared_ptr<IFileSystem> fileSystem = std::make_shared<FileSystem>(exePath);
    std::shared_ptr<IConfigFile> configFile = std::make_shared<ConfigFile>(fileSystem);
    std::shared_ptr<BackgroundTileAssetManager> backgroundTileAssetManager =  std::make_shared<BackgroundTileAssetManager>(configFile);
    std::shared_ptr<TiledWorld> level = std::make_shared<TiledWorld>(configFile, backgroundTileAssetManager);
    InputManager inputManager(configFile);
    level->LoadLevel(0);
    

    uvec2 windowSize = level->GetRequiredBaseWindowSize();
    float scaleFactor = 3.0f;
    const int SCREEN_WIDTH = windowSize.x * scaleFactor;
    const int SCREEN_HEIGHT = windowSize.y * scaleFactor;
    
    
    //Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    }
    else
    {
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
            std::shared_ptr<IAnimationAssetManager> animationAssetManager = std::make_shared<AnimationAssetManager>(configFile, screenSurface);
            Character character(animationAssetManager, configFile, level);
            

            //Hack to get window to stay up
            bool quit = false; 
            u32 a = SDL_GetTicks();
            u32 b = SDL_GetTicks();
            double delta = 0;
            while (quit == false) 
            {
                a = SDL_GetTicks();
                delta += a - b;

                if (delta > 1000 / 60.0)
                {
                    b = a;
                    //std::cout << "fps: " << 1000 / delta << std::endl;
                    GameInputState state = inputManager.PollEvents();
                    character.Update(delta, state);

                    //Fill the surface white
                    SDL_FillSurfaceRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0xFF, 0xFF, 0xFF));
                    level->DrawActiveLevel(screenSurface, scaleFactor);
                    character.Draw(screenSurface, scaleFactor);
                    //Update the surface
                    SDL_UpdateWindowSurface(window);
                    delta = 0;
                }
            }
        }
    }
    //Destroy window
    SDL_DestroyWindow(window);

    //Quit SDL subsystems
    SDL_Quit();

    return 0;
}