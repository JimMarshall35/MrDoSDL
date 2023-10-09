#include "AppleManager.h"
#include "IConfigFile.h"
#include "Event.h"
#include <iostream>


AppleManager::AppleManager(
	const std::shared_ptr<IAnimationAssetManager>& assetManager,
	const std::shared_ptr<IConfigFile>& configFile,
	const std::shared_ptr<TiledWorld>& tiledWorld,
	Event<int>& onNewLevelStarted
)
	:ApplePoolSize(configFile->GetUIntValue("ApplePoolSize")),
	ApplePool(std::make_unique<Apple[]>(ApplePoolSize)),
	LOnNewLevelStarted(this)
{
	onNewLevelStarted += &LOnNewLevelStarted;
}

void AppleManager::Update(float deltaT)
{
}

void AppleManager::Draw(SDL_Surface* windowSurface, float scale) const
{
}

void AppleManager::OnNewLevelStarted(int levelNumber)
{
	std::cerr << "started\n";
}
