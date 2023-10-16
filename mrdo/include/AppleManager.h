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
		Inactive,
		Settled,
		Wobbling,
		Falling,
		Splitting,
		Sliding
	};
	struct Apple
	{
		vec2 Position;
		AppleState State = AppleState::Inactive;
		float SlideDestination;
		float AnimationTimer = 0;;
		u32 OnAnimationFrame;
		float DistanceFallen = 0.0f;
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
	void RecursivelyPushApples(Apple& applePushed);
	void ClampPushedApples();
private:
	std::shared_ptr<IConfigFile> CachedConfig;
	const std::shared_ptr<IAnimationAssetManager> AnimationAssetManager;
	u32 ApplePoolSize;
	u32 ThisLevelNumApplesAtStart;
	std::unique_ptr<Apple[]> ApplePool;
	float CachedBackgroundTileSize;
	std::vector<SDL_Rect> WobbleAnimation;
	std::vector<SDL_Rect> LeftHalfSplitAnimation;
	std::vector<SDL_Rect> RightHalfSplitAnimation;
	Character* CharacterRef;
private:
	void OnNewLevelStarted(int levelNumber);
	bool IsCellBelowEmpty(Apple* apple) const;
	bool IsCellDirectlyBelowEmpty(Apple* apple) const;
private:
	LISTENER(AppleManager, OnNewLevelStarted, int);
private:
	vec2 CachedSpriteDims;
	uvec2 CachedLevelSize = { 0,0 };


	std::unique_ptr<Apple*[]> PushedAppleStack;
	int PushedAppleStackSize = 0;
	const std::shared_ptr<TiledWorld> CachedTiledWorld;
	float AppleSlideSpeed;
	float AppleFallSpeed;
	float AppleWobbleTime;
	float AppleSplitTime;
};