#pragma once
#include <memory>
#include <vector>
#include <functional>
#include "VectorTypes.h"
#include "EventListener.h"
#include "SDL.h"
#include "MovementTypes.h"
#include "LevelLoadData.h"


class IAnimationAssetManager;
class TiledWorld;
class Character;
class IConfigFile;

template<typename T>
class Event;

struct Enemy;
class EnemyManager;

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
		float AnimationTimer = 0;
		u32 OnAnimationFrame;
		float DistanceFallen = 0.0f;
		Character* CrushedCharacter = nullptr;
		std::unique_ptr<Enemy*[]> EnemyBuffer;
		u32 EnemyBufferCurrentSize = 0;
	};
	struct SettledAppleEnemyCollision
	{
		CollidingCellRelationship CellRelationship;
		Enemy* CollidedEnemy;
	};
public:
	AppleManager(
		const std::shared_ptr<IAnimationAssetManager>& assetManager, 
		const std::shared_ptr<IConfigFile>& configFile, 
		const std::shared_ptr<TiledWorld>& tiledWorld,
		Event<LevelLoadData>& onNewLevelStarted,
		EnemyManager* enemyManager);
	void Update(float deltaT);
	void Draw(SDL_Surface* windowSurface, float scale) const;
	void SetCharacter(Character* character);
	Apple* FindActiveAppleByPredicate(std::function<bool(const Apple&)> predicate);
private:
	void UpdateSingleApple(float deltaT, Apple& apple);
	void RecursivelyPushApples(Apple& applePushed);
	void ClampPushedApples(bool pushedByEnemy = false);
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
	std::unique_ptr<SettledAppleEnemyCollision[]> EnemyCollisionRelationships;
	size_t NumCollidingEnemies = 0;
	size_t MaxMonstersThisLevel = 0;
private:
	void OnNewLevelStarted(LevelLoadData levelNumber);
	bool IsCellBelowEmpty(Apple* apple) const;
	bool IsCellDirectlyBelowEmpty(Apple* apple) const;
	bool IsMrDoBelow(Apple* apple) const;
	bool IsAppleBelow(Apple* apple) const;
	vec2 GetCellBelowPos(Apple* apple) const;
	CollidingCellRelationship GetCollisionRelationshipBase(Apple* apple, const vec2& pos, const vec2& dims) const;
	CollidingCellRelationship GetCollisionRelationshipWithMrDo(Apple* apple) const;
	void PopulateEnemyCollisionRelationships(Apple* apple);
	bool ResolveEnemyCollisions(Apple* apple);
private:
	LISTENER(AppleManager, OnNewLevelStarted, LevelLoadData);
private:
	vec2 CachedSpriteDims;
	ivec2 CachedLevelSize = { 0,0 };
	std::unique_ptr<Apple*[]> PushedAppleStack;
	int PushedAppleStackSize = 0;
	const std::shared_ptr<TiledWorld> CachedTiledWorld;
	float AppleSlideSpeed;
	float AppleFallSpeed;
	float AppleWobbleTime;
	float AppleSplitTime;
	EnemyManager* CachedEnemyManager;
};