#pragma once
#include <memory>
#include "CommonTypedefs.h"
#include "VectorTypes.h"
#include "LevelLoadData.h"
#include "EventListener.h"
#include "Event.h"
#include "MovementTypes.h"
#include "SDL.h"
#include <vector>

class IConfigFile;
class IAnimationAssetManager;

class EnemyManager
{
private:
	enum class EnemyType
	{
		Normal,
		Digger,
		ExtraMan,
		Ghost
	};
	enum class EnemySpawnerState
	{
		Idle,
		Flashing
	};
	struct EnemySpawner
	{
		u32 NumberOfEnemiesLeftToSpawn;
		uvec2 TileCoords;
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
		MovementDirection CurrentDirection;
		u8 bPushing : 1;
		u8 bActive : 1;
	};
public:
	EnemyManager(
		const std::shared_ptr<IConfigFile>& configFile,
		const std::shared_ptr<IAnimationAssetManager>& animationAssetManager,
		Event<LevelLoadData>& onLevelLoaded,
		Event<LevelLoadData>& onResetAfterDeath);
	void Update(float deltaTime);
	void Draw(SDL_Surface* windowSurface, float scale) const;

private:
	void OnLevelBegun(LevelLoadData level);
	void OnResetAfterDeath(LevelLoadData level);
	void PopulateAnimationTables();
	void UpdateSingleSpawner(float deltaTime, EnemySpawner& spawner);
	void SpawnEnemy(EnemySpawner& spawner);

private:
	LISTENER(EnemyManager, OnLevelBegun, LevelLoadData);
	LISTENER(EnemyManager, OnResetAfterDeath, LevelLoadData);

private:
	std::shared_ptr<IConfigFile> ConfigFile;
	std::shared_ptr<IAnimationAssetManager> AnimationAssetManager;
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
	static SDL_Rect SpawnerTileSprite;
};