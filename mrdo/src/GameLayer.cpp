#include "GameLayer.h"
#include "TiledWorld.h"
#include "TextRenderer.h"
#include <cassert>
#include <IConfigFile.h>
#include "BackendClient.h"
#include "InputManager.h"

std::string Game::LayerName = "Game";
#ifdef ReplayValidator
Game::Game(const std::shared_ptr<IFileSystem>& fileSystem,
	const std::shared_ptr<IConfigFile>& config, 
	const std::shared_ptr<IBackgroundTileAssetManager>& backgroundAssetManager, 
	const std::shared_ptr<IAnimationAssetManager>& animationManager,
	InputManager* inputManager,
	std::function<void(void)>& gameOverCallback)
	:MyTiledWorld(std::make_shared<TiledWorld>(config, backgroundAssetManager, animationManager)),
	MyCharacter(animationManager, config, MyTiledWorld, NewLevelBegun, ResetAfterDeath, &MyEnemyManager),
	MyAppleManager(animationManager, config, MyTiledWorld, NewLevelBegun, &MyEnemyManager),
	MyEnemyManager(config, animationManager, MyTiledWorld.get(), NewLevelBegun, ResetAfterDeath, &MyCharacter),
	Phase(GamePhase::Playing),
	MyGameState(config, NewLevelBegun, ResetAfterDeath),
	Config(config),
	MyInputManager(inputManager),
	OnGameOverCallback(gameOverCallback)
{
	MyCharacter.SetAppleManager(&MyAppleManager);
	MyAppleManager.SetCharacter(&MyCharacter);
}

#else
Game::Game(const std::shared_ptr<IFileSystem>& fileSystem,
	const std::shared_ptr<IConfigFile>& config,
	const std::shared_ptr<IBackgroundTileAssetManager>& backgroundAssetManager,
	const std::shared_ptr<IAnimationAssetManager>& animationManager,
	const std::shared_ptr<TextRenderer>& textRenderer,
	const std::shared_ptr<IBackendClient>& backendClient,
	InputManager* inputManager)
	:MyTiledWorld(std::make_shared<TiledWorld>(config, backgroundAssetManager, animationManager)),
	MyCharacter(animationManager, config, MyTiledWorld, NewLevelBegun, ResetAfterDeath, &MyEnemyManager),
	MyAppleManager(animationManager, config, MyTiledWorld, NewLevelBegun, &MyEnemyManager),
	MyEnemyManager(config, animationManager, MyTiledWorld.get(), NewLevelBegun, ResetAfterDeath, &MyCharacter),
	Phase(GamePhase::Playing),
	MyGameState(config,textRenderer, NewLevelBegun, ResetAfterDeath),
	Config(config),
	CachedTextRenderer(textRenderer),
	BackendClient(backendClient),
	MyInputManager(inputManager)
{
	MyCharacter.SetAppleManager(&MyAppleManager);
	MyAppleManager.SetCharacter(&MyCharacter);
}
#endif

void Game::Update(float deltaT)
{
	switch (Phase)
	{
	case GamePhase::Playing:
		MyCharacter.Update(deltaT, InputState);
		MyEnemyManager.Update(deltaT);
		MyAppleManager.Update(deltaT);
		break;
	case GamePhase::DieAnimationPlaying:
		MyCharacter.UpdatePlayingDeathAnimation(deltaT);
		if (MyCharacter.IsAnimationFinished())
		{
			ResetAfterDeath(CurrentLevel);
			Phase = GamePhase::Playing;
		}
		break;
	}
}

bool Game::MasksPreviousUpdateableLayer() const
{
	return true;
}

const std::string& Game::GetUpdateableLayerName() const
{
	return LayerName;
}

void Game::OnUpdatePush(void* data)
{
	const LevelLoadData* level = (const LevelLoadData*)data;
	MyTiledWorld->LoadLevel(level);
	NewLevelBegun(*level);
	CurrentLevel = *level;
	InputState = { false, false, false, false, true, false };
	
}

void Game::OnUpdatePop()
{
}

void Game::Draw(SDL_Surface* windowSurface, float scale) const
{
#ifndef ReplayValidator
	MyTiledWorld->DrawActiveLevel(windowSurface, scale);
	MyCharacter.Draw(windowSurface, scale);
	MyAppleManager.Draw(windowSurface, scale);
	MyEnemyManager.Draw(windowSurface, scale);
	MyGameState.Draw(windowSurface, scale);
	
#endif
	//CachedTextRenderer->RenderText({ 0,0 }, "HeLlO WoRlD ^", windowSurface, scale);
}

bool Game::MasksPreviousDrawableLayer() const
{
	return true;
}

const std::string& Game::GetDrawableLayerName() const
{
	return LayerName;
}

void Game::OnDrawablePush(void* data)
{
#ifndef ReplayValidator
	CachedTextRenderer->SetCurrentFont("White_BlackBackground");
#endif
}

void Game::OnDrawablePop()
{
}

void Game::ReceiveInput(const GameInputState& input)
{
	InputState = input;
	if (input.BackPress())
	{
		GameFramework::QueuePopLayersAtFrameEnd(GameLayerType::Draw | GameLayerType::Input | GameLayerType::Update);
	}
}

bool Game::MasksPreviousInputLayer() const
{
	return true;
}

const std::string& Game::GetInputLayerName() const
{
	return LayerName;
}

void Game::OnInputPush(void* data)
{
	const LevelLoadData* level = (const LevelLoadData*)data;
	if (level->Source == LevelSource::ArcadeLevels && level->LevelIndex == 0)
	{
#ifdef ReplayValidator
		MyInputManager->SetRecordingState(InputRecordingState::PlayingBack);
#else
		MyInputManager->SetRecordingState(InputRecordingState::Recording);

#endif
	}
}

void Game::OnInputPop()
{
	if (CurrentLevel.Source == LevelSource::ArcadeLevels)
	{
		MyInputManager->SetRecordingState(InputRecordingState::NotRecording);
	}
}

void Game::RecieveMessage(const CharacterDied& message)
{
	assert(Phase == GamePhase::Playing);
	Phase = GamePhase::DieAnimationPlaying;
}

void Game::RecieveMessage(const Victory& message)
{
	switch (CurrentLevel.Source)
	{
	case LevelSource::ArcadeLevels:
		{
			int numLevels = Config->GetLevelsConfigData().size();
			CurrentLevel.LevelIndex++;
			if (CurrentLevel.LevelIndex >= numLevels)
			{
				CurrentLevel.LevelIndex = 0;
			}
			MyTiledWorld->LoadLevel(&CurrentLevel);
			NewLevelBegun(CurrentLevel);
		}
		break;
	case LevelSource::MapMaker:
		{

		}
		break;
	}
	
}

void Game::RecieveMessage(const GameOver& message)
{
#ifndef ReplayValidator
	BackendClient->SubmitPossibleHighScore(message.Score);
	MyInputManager->SaveRecordingFile();
	MyInputManager->SetRecordingState(InputRecordingState::NotRecording);
	GameFramework::QueuePopLayersAtFrameEnd(GameLayerType::Draw | GameLayerType::Input | GameLayerType::Update);
#else
	OnGameOverCallback();
#endif
}
