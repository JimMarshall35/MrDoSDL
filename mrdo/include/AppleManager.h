#pragma once
#include <memory>
#include <vector>
#include "VectorTypes.h"
#include "EventListener.h"
#include "SDL.h"

class IConfigFile;
class IAnimationAssetManager;
class TiledWorld;

template<typename T>
class Event;

struct LevelConfigData;

class AppleManager
{
private:
	struct Apple
	{
		vec2 Position;
		bool bIsActive = true;
	};
public:
	AppleManager(
		const std::shared_ptr<IAnimationAssetManager>& assetManager, 
		const std::shared_ptr<IConfigFile>& configFile, 
		const std::shared_ptr<TiledWorld>& tiledWorld,
		Event<int>& onNewLevelStarted);
	void Update(float deltaT);
	void Draw(SDL_Surface* windowSurface, float scale) const;
private:
	std::shared_ptr<IConfigFile> CachedConfig;
	const std::shared_ptr<IAnimationAssetManager> AnimationAssetManager;
	u32 ApplePoolSize;
	u32 ThisLevelNumApplesAtStart;
	std::unique_ptr<Apple[]> ApplePool;
	float CachedBackgroundTileSize;
	std::vector<SDL_Rect> WobbleAnimation;
private:
	void OnNewLevelStarted(int levelNumber);
	LISTENER(AppleManager, OnNewLevelStarted, int);
};