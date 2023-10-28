#include "GameLayer.h"
#include "TiledWorld.h"
#include "TextRenderer.h"
#include <cassert>

std::string Game::LayerName = "Game";

Game::Game(const std::shared_ptr<IFileSystem>& fileSystem, 
	const std::shared_ptr<IConfigFile>& config,
	const std::shared_ptr<IBackgroundTileAssetManager>& backgroundAssetManager,
	const std::shared_ptr<IAnimationAssetManager>& animationManager,
	const std::shared_ptr<TextRenderer>& textRenderer)
	:MyTiledWorld(std::make_shared<TiledWorld>(config, backgroundAssetManager, animationManager)),
	MyCharacter(animationManager, config, MyTiledWorld, NewLevelBegun, ResetAfterDeath),
	MyAppleManager(animationManager, config, MyTiledWorld, NewLevelBegun),
	Phase(GamePhase::Playing),
	CachedTextRenderer(textRenderer)
{
	MyCharacter.SetAppleManager(&MyAppleManager);
	MyAppleManager.SetCharacter(&MyCharacter);
}

void Game::Update(float deltaT)
{
	switch (Phase)
	{
	case GamePhase::Playing:
		MyCharacter.Update(deltaT, InputState);
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
	MyTiledWorld->DrawActiveLevel(windowSurface, scale);
	MyCharacter.Draw(windowSurface, scale);
	MyAppleManager.Draw(windowSurface, scale);
	CachedTextRenderer->RenderText({ 0,0 }, "HeLlO WoRlD ^", windowSurface, scale);
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
	CachedTextRenderer->SetCurrentFont("White_BlackBackground");
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
	
}

void Game::OnInputPop()
{
}

void Game::RecieveMessage(const CharacterDied& message)
{
	assert(Phase == GamePhase::Playing);
	Phase = GamePhase::DieAnimationPlaying;
}
