#include "AppleManager.h"
#include "IConfigFile.h"
#include "Event.h"
#include <iostream>
#include <cassert>
#include "IAnimationAssetManager.h"


AppleManager::AppleManager(
	const std::shared_ptr<IAnimationAssetManager>& assetManager,
	const std::shared_ptr<IConfigFile>& configFile,
	const std::shared_ptr<TiledWorld>& tiledWorld,
	Event<int>& onNewLevelStarted
)
	:CachedConfig(configFile),
	AnimationAssetManager(assetManager),
	ApplePoolSize(configFile->GetUIntValue("ApplePoolSize")),
	ThisLevelNumApplesAtStart(0),
	ApplePool(std::make_unique<Apple[]>(ApplePoolSize)),
	LOnNewLevelStarted(this)
{
	onNewLevelStarted += &LOnNewLevelStarted;
	CachedBackgroundTileSize = configFile->GetBackgroundConfigData().TileSize;
	assetManager->MakeAnimationRectFramesFromName("Apple_Wobble", WobbleAnimation);
}

void AppleManager::Update(float deltaT)
{
}

void AppleManager::Draw(SDL_Surface* windowSurface, float scale) const
{
	SDL_Surface* surface = AnimationAssetManager->GetAnimationsSpriteSheetSurface();
	for (int i = 0; i < ThisLevelNumApplesAtStart; i++)
	{
		const Apple& apple = ApplePool[i];
		if (apple.bIsActive)
		{
			SDL_Rect dst;
			dst.w = CachedBackgroundTileSize * scale;
			dst.h = CachedBackgroundTileSize * scale;
			dst.x = (float)apple.Position.x * scale;
			dst.y = (float)apple.Position.y * scale;

			const SDL_Rect& rect = WobbleAnimation[1];//Animator.GetCurrentFrame();
			SDL_BlitSurfaceScaled(surface, &rect, windowSurface, &dst);
		}
	}
}

void AppleManager::OnNewLevelStarted(int levelNumber)
{
	const std::vector<LevelConfigData>& levels = CachedConfig->GetLevelsConfigData();
	const LevelConfigData& startedLevel = levels[levelNumber];
	ThisLevelNumApplesAtStart = startedLevel.Apples.size();
	assert(ThisLevelNumApplesAtStart <= ApplePoolSize);
	for (int i = 0; i < ThisLevelNumApplesAtStart; i++)
	{
		ApplePool[i].bIsActive = true;
		ApplePool[i].Position = vec2{ (float)startedLevel.Apples[i].x * CachedBackgroundTileSize, (float)startedLevel.Apples[i].y * CachedBackgroundTileSize };
	}
	std::cerr << "started\n";
}
