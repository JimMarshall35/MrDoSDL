#pragma once
#include <memory>
#include "CommonTypedefs.h"
#include "VectorTypes.h"
#include "LevelLoadData.h"
#include "EventListener.h"
#include "Event.h"
#include "MovementTypes.h"
#include "SDL.h"
#include "Animator.h"
#include "ForthCommonTypedefs.h"
#include <vector>
#include <functional>
#include "EnemyScripting.h"
#include "EnemyType.h"

class IConfigFile;
class IAnimationAssetManager;
class Character;
class TiledWorld;

enum class EnemySpawnerState
{
	Idle,
	Flashing
};
struct EnemySpawner
{
	u32 NumberOfEnemiesLeftToSpawn;
	ivec2 TileCoords;
	EnemySpawnerState State;
	float Timer;
	u32 FlashCounter;
	u8 bFlashState : 1;
};
struct Enemy
{
	EnemySpawner* OriginSpawner;
	EnemyType Type;
	vec2 Pos;
	vec2 Destination;
	MovementDirection CurrentDirection;
	std::unique_ptr<ivec2[]> PathBuffer;
	u32 PathBufferCurrentSize;
	i32 PathBufferDestinationIndex;
	ivec2 CurrentCell;
	Animator EnemyAnimator;
	Cell Timer;
	u8 bPushing : 1;
	u8 bActive : 1;
	u8 bCrushed : 1;
};
typedef std::function<void(Enemy&)> EnemyIterator;

class EnemyManager
{
public:
	friend class EnemyScripting::EnemyManager_ForthExposedMethodImplementations;
public:
	EnemyManager(
		const std::shared_ptr<IConfigFile>& configFile,
		const std::shared_ptr<IAnimationAssetManager>& animationAssetManager,
		TiledWorld* tiledWorld,
		Event<LevelLoadData>& onLevelLoaded,
		Event<LevelLoadData>& onResetAfterDeath,
		Character* character);
	void Update(float deltaTime);
	void Draw(SDL_Surface* windowSurface, float scale) const;
	void IterateActiveEnemies(EnemyIterator iter) const;
	int GetActiveLevelMaxEnemies() const;
	void CrushEnemy(Enemy* enemy);
	void KillEnemy(Enemy* enemy);
	void SetEnemyPushingState(Enemy* enemy, bool newState);
private:
	typedef std::function<void(Enemy&)> PathFinishedCallback;

private:
	void OnLevelBegun(LevelLoadData level);
	void OnResetAfterDeath(LevelLoadData level);
	void PopulateAnimationTables();
	void UpdateSingleSpawner(float deltaTime, EnemySpawner& spawner);
	void SpawnEnemy(EnemySpawner& spawner);
	void UpdateSingleEnemy(float deltaTime, Enemy& enemy);
	void InitialiseEnemyPool();
	void SetNewPath(Enemy& enemy, const ivec2& newDestinationCell);
	void SetNewPathForDigger(Enemy& enemy, const ivec2& newDestinationCell, const ivec2& obstruction);
	void SetEnemyDirection(Enemy& enemy);
	void SetEnemyDestinationWorldSpace(Enemy& enemy);

	// true == passed into new cell
	bool FollowPathBase(Enemy& enemy, float deltaT, const PathFinishedCallback& onPathFinished, float speedMultiplier=1.0f);	

private:
	LISTENER(EnemyManager, OnLevelBegun, LevelLoadData);
	LISTENER(EnemyManager, OnResetAfterDeath, LevelLoadData);

private:
	std::shared_ptr<IConfigFile> ConfigFile;
	std::shared_ptr<IAnimationAssetManager> AnimationAssetManager;
	TiledWorld* CachedTiledWorld;
	u32 EnemySpawnerPoolSize;
	u32 EnemyPoolSize;
	std::unique_ptr<EnemySpawner[]> EnemySpawnerPool;
	std::unique_ptr<Enemy[]> EnemyPool;
	i32 BackgroundTileSize;
	float SpawnEnemyInterval;
	float SpawnEnemyFlashInterval;
	u32 FlashesBeforeSpawn;
	
	u32 NumEnemySpawnersThisLevel = 0;
	u32 MaxEnemiesThisLevel = 0;
	u32 NumEnemiesSpawned = 0;
	static std::vector<SDL_Rect> NormalEnemyAnimationTable[2][4];
	static std::vector<SDL_Rect> DiggerAnimationTable[4];
	static std::vector<SDL_Rect> TransformingToDiggerAnimationTable[4];
	static SDL_Rect SpawnerTileSprite;
	static SDL_Rect CrushedMonsterSprite;
	Character* CachedCharacter;
	u32 PathBufferSize;
	float EnemySpeed;
	float CachedTileSize;

	float DigSpeedMultiplier;
	float PushSpeedMultiplier;
	float EnemyWaitTimeBeforeBecomeDigger;
	float MorphingEnemyFlashAnimationFPS;
	float MorphingEnemyFlashTime;
	static float DeltaTime;
	vec2 EnemyCollider;
	vec2 PlayerCollider;

	ExecutionToken UpdateNormalEnemyScriptFunction;
	ExecutionToken FlashingEnemyScriptFunction;
	ExecutionToken DiggerEnemyScriptFunction;
};