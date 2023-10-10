#include "GameLayer.h"
#include "TiledWorld.h"

std::string Game::LayerName = "Game";

Game::Game(const std::shared_ptr<IFileSystem>& fileSystem, const std::shared_ptr<IConfigFile>& config, const std::shared_ptr<IBackgroundTileAssetManager>& backgroundAssetManager, const std::shared_ptr<IAnimationAssetManager>& animationManager)
	:MyTiledWorld(std::make_shared<TiledWorld>(config, backgroundAssetManager)),
	MyCharacter(animationManager, config, MyTiledWorld, NewLevelBegun),
	MyAppleManager(animationManager, config, MyTiledWorld, NewLevelBegun)
{
	MyCharacter.SetAppleManager(&MyAppleManager);
	MyAppleManager.SetCharacter(&MyCharacter);
}

void Game::Update(float deltaT)
{
	MyCharacter.Update(deltaT, InputState);
	MyAppleManager.Update(deltaT);
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
	int level = (int)data;
	MyTiledWorld->LoadLevel(level);
	NewLevelBegun(level);
}

void Game::OnUpdatePop()
{
}

void Game::Draw(SDL_Surface* windowSurface, float scale) const
{
	MyTiledWorld->DrawActiveLevel(windowSurface, scale);
	MyCharacter.Draw(windowSurface, scale);
	MyAppleManager.Draw(windowSurface, scale);
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
}

void Game::OnDrawablePop()
{
}

void Game::ReceiveInput(const GameInputState& input)
{
	InputState = input;
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
