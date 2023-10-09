#pragma once
#include <memory>
#include "VectorTypes.h"
#include "EventListener.h"

class IConfigFile;
class IAnimationAssetManager;
class TiledWorld;
struct SDL_Surface;

template<typename T>
class Event;

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
	u32 ApplePoolSize;
	std::unique_ptr<Apple[]> ApplePool;
	void OnNewLevelStarted(int levelNumber);
	LISTENER(AppleManager, OnNewLevelStarted, int);
};