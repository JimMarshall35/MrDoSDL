#pragma once
#include <memory>
#include <vector>
#include <functional>
#include "VectorTypes.h"
#include "EventListener.h"
#include "SDL.h"

class IConfigFile;
class IAnimationAssetManager;
class TiledWorld;
class Character;

template<typename T>
class Event;

struct LevelConfigData;

class AppleManager
{
public:
	enum class AppleState
	{
		Settled,
		Wobbling,
		Falling,
		Splitting
	};
	struct Apple
	{
		vec2 Position;
		bool bIsActive = true;
		AppleState State = AppleState::Settled;
	};
public:
	AppleManager(
		const std::shared_ptr<IAnimationAssetManager>& assetManager, 
		const std::shared_ptr<IConfigFile>& configFile, 
		const std::shared_ptr<TiledWorld>& tiledWorld,
		Event<int>& onNewLevelStarted);
	void Update(float deltaT);
	void Draw(SDL_Surface* windowSurface, float scale) const;
	void SetCharacter(Character* character);
	Apple* FindActiveAppleByPredicate(std::function<bool(const Apple&)> predicate);
private:
	void UpdateSingleApple(float deltaT, Apple& apple);
private:
	std::shared_ptr<IConfigFile> CachedConfig;
	const std::shared_ptr<IAnimationAssetManager> AnimationAssetManager;
	u32 ApplePoolSize;
	u32 ThisLevelNumApplesAtStart;
	std::unique_ptr<Apple[]> ApplePool;
	float CachedBackgroundTileSize;
	std::vector<SDL_Rect> WobbleAnimation;
	Character* CharacterRef;
private:
	void OnNewLevelStarted(int levelNumber);
private:
	LISTENER(AppleManager, OnNewLevelStarted, int);
	vec2 CachedSpriteDims;

};