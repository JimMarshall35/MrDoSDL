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
#include "EnemyScripting.h"
#include "BackendClient.h"
#include <functional>
#include <curl/curl.h>

#ifndef ReplayValidator
int GameMain(int argc, char* args[])
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
        ivec2 windowSize = { 12 * 16, 15 * 16 }; //level->GetRequiredBaseWindowSize();
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
            std::shared_ptr<IBackendClient> backendClient = std::make_shared<BackendClient>(configFile);
            backendClient->PopulateHighScores();
            InputManager inputManager(configFile, fileSystem);
            inputManager.LoadRecordingFile();

            std::shared_ptr<IAnimationAssetManager> animationAssetManager = std::make_shared<AnimationAssetManager>(configFile, screenSurface);

            PathFinding::Initialise(configFile->GetUIntValue("PathFindingPriorityQueueSize"));
            EnemyScripting::InitScripting(
                configFile->GetUIntValue("EnemyScriptingVMDictionarySizeCells"),
                configFile->GetUIntValue("EnemyScriptingVMIntStackSizeCells"),
                configFile->GetUIntValue("EnemyScriptingVMReturnStackSizeCells"));
            EnemyScripting::EnemyManager_ForthExposedMethodImplementations::RegisterForthFunctions();
            EnemyScripting::DoFile(fileSystem->GetEnemyAIFilePath());
            EnemyScripting::ForthDoString("showWords");

            // game framework layers
            Game game(fileSystem, configFile, backgroundTileAssetManager, animationAssetManager, textRenderer, backendClient, &inputManager);
            FrontEndLayer frontend(textRenderer, SCREEN_WIDTH, SCREEN_HEIGHT);
            MapMakerLevelSelectLayer mapMakerLevelSelect(textRenderer, configFile, SCREEN_WIDTH, SCREEN_HEIGHT);
            MapMakerCreateNewLevelDialogue mapMakerCreateNewLevelDialogue(textRenderer, configFile, SCREEN_WIDTH, SCREEN_HEIGHT);
            MapMakerLevelSelectedDialogue mapMakerLevelSelectedDialogue(configFile, textRenderer, SCREEN_WIDTH, SCREEN_HEIGHT);
            MapMakerLayer MapMakerLayer(configFile, backgroundTileAssetManager, animationAssetManager, textRenderer);

            GameFramework::PushLayers("FrontEnd", GameLayerType::Draw | GameLayerType::Input | GameLayerType::Update, 0);

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
            EnemyScripting::DeInitScripting();
            PathFinding::DeInit();
        }
    }
    //Destroy window
    SDL_DestroyWindow(window);

    //Quit SDL subsystems
    SDL_Quit();


    return 0;

}
#else
int ReplayValidatorMain(int argc, char* args[])
{

    std::string replayFileName = std::string(args[1]);
    u32 expectedScore = std::atoi(args[2]);
    std::cout << "\n\n\n\n\n Mr Do! Replay validator.\n";
   std::cout << "Replay file name: " << replayFileName << "\n";
   std::cout << "Expected score: " << expectedScore << "\n";
   char* exePath = args[0];

   LevelLoadData LevelLoad = { LevelSource::ArcadeLevels, 0 };
   std::shared_ptr<IFileSystem> fileSystem = std::make_shared<FileSystem>(exePath);
   std::shared_ptr<IConfigFile> configFile = std::make_shared<ConfigFile>(fileSystem);
   std::shared_ptr<BackgroundTileAssetManager> backgroundTileAssetManager = std::make_shared<BackgroundTileAssetManager>(configFile);
   InputManager inputManager(configFile, fileSystem);
   std::shared_ptr<IAnimationAssetManager> animationAssetManager = std::make_shared<AnimationAssetManager>(configFile);
   bool bContinue = true;
   inputManager.LoadRecordingFile(replayFileName);
   std::function<void(void)> cb = [&bContinue]()
   {
       bContinue = false; 
   };

   PathFinding::Initialise(configFile->GetUIntValue("PathFindingPriorityQueueSize"));
   EnemyScripting::InitScripting(
       configFile->GetUIntValue("EnemyScriptingVMDictionarySizeCells"),
       configFile->GetUIntValue("EnemyScriptingVMIntStackSizeCells"),
       configFile->GetUIntValue("EnemyScriptingVMReturnStackSizeCells"));
   EnemyScripting::EnemyManager_ForthExposedMethodImplementations::RegisterForthFunctions();
   EnemyScripting::DoFile(fileSystem->GetEnemyAIFilePath());


   Game game(fileSystem, configFile, backgroundTileAssetManager, animationAssetManager, &inputManager, cb);
   GameFramework::PushLayers("Game", GameLayerType::Draw | GameLayerType::Input | GameLayerType::Update, &LevelLoad);

   while (bContinue) { 
       GameInputState state = inputManager.PollEvents();
       bContinue = !state.Quit;
       GameFramework::RecieveInput(state);
       GameFramework::Update(16);
       GameFramework::EndFrame();
   }
   EnemyScripting::DeInitScripting();
   PathFinding::DeInit();
   u32 score = game.GetGamestate().GetScore();
   std::string s = std::string((score == expectedScore) ? " equals expectedScore " : " does not equal expected score ");
   std::cout << "Game Score: " << score << s << expectedScore << "\n";
   if (score == expectedScore)
   {
       std::cout << "REPLAY VALID\n";
   }
   else
   {
       std::cout << "REPLAY INVALID\n";
   }
   //Game game(fileSystem, configFile,backgr)
    return expectedScore != score;
}
#endif

int main(int argc, char* args[])
{
#ifdef ReplayValidator
    return ReplayValidatorMain(argc, args);
#else
    return GameMain(argc, args);
#endif
}