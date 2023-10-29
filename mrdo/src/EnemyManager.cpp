#include "EnemyManager.h"
#include "IAnimationAssetManager.h"
#include "IConfigFile.h"

std::vector<SDL_Rect> EnemyManager::NormalEnemyAnimationTable[2][4];
SDL_Rect EnemyManager::SpawnerTileSprite;

EnemyManager::EnemyManager(
	const std::shared_ptr<IConfigFile>& configFile,
	const std::shared_ptr<IAnimationAssetManager>& animationAssetManager,
	Event<LevelLoadData>& onLevelLoaded,
	Event<LevelLoadData>& onResetAfterDeath)
	:LOnLevelBegun(this),
	LOnResetAfterDeath(this),
	ConfigFile(configFile),
	AnimationAssetManager(animationAssetManager),
	EnemySpawnerPoolSize(configFile->GetUIntValue("MonsterSpawnerPoolSize")),
	EnemyPoolSize(configFile->GetUIntValue("MonsterPoolSize")),
	EnemySpawnerPool(std::make_unique<EnemySpawner[]>(EnemySpawnerPoolSize)),
	EnemyPool(std::make_unique<Enemy[]>(EnemyPoolSize)),
	BackgroundTileSize(configFile->GetBackgroundConfigData().TileSize),
	SpawnEnemyInterval(configFile->GetFloatValue("SpawnEnemyInterval")),
	SpawnEnemyFlashInterval(configFile->GetFloatValue("SpawnEnemyFlashInterval")),
	FlashesBeforeSpawn(configFile->GetUIntValue("FlashesBeforeSpawn"))
{
	onLevelLoaded += &LOnLevelBegun;
	onResetAfterDeath += &LOnResetAfterDeath;
	AnimationAssetManager->MakeSingleSpriteRectFrame("MonsterSpawner", SpawnerTileSprite);
	PopulateAnimationTables();
}

void EnemyManager::Update(float deltaTime)
{
	for (int i = 0; i < NumEnemySpawnersThisLevel; i++)
	{
		EnemySpawner& spawner = EnemySpawnerPool[i];
		if (spawner.NumberOfEnemiesLeftToSpawn > 0)
		{
			UpdateSingleSpawner(deltaTime, spawner);
		}
	}
}

void EnemyManager::Draw(SDL_Surface* windowSurface, float scale) const
{
	for (int i = 0; i < NumEnemySpawnersThisLevel; i++)
	{
		const EnemySpawner& spawner = EnemySpawnerPool[i];
		if (spawner.NumberOfEnemiesLeftToSpawn > 0)
		{
			SDL_Rect dst;
			SDL_Surface* srcSurface = AnimationAssetManager->GetAnimationsSpriteSheetSurface();
			dst.w = BackgroundTileSize * scale;
			dst.h = BackgroundTileSize * scale;
			dst.x = spawner.TileCoords.x * BackgroundTileSize * scale;
			dst.y = spawner.TileCoords.y * BackgroundTileSize * scale;
			if (spawner.State == EnemySpawnerState::Idle)
			{
				SDL_BlitSurfaceScaled(srcSurface, &SpawnerTileSprite, windowSurface, &dst);
			}
			else if (spawner.State == EnemySpawnerState::Flashing)
			{
				if (!spawner.bFlashState)
				{
					SDL_BlitSurfaceScaled(srcSurface, &SpawnerTileSprite, windowSurface, &dst);
				}
				else
				{
					SDL_BlitSurfaceScaled(srcSurface, &NormalEnemyAnimationTable[0][(i32)MovementDirection::Right][0], windowSurface, &dst);
				}
			}
		}
		else
		{
			// draw the bonus item here 
		}
	}
}

void EnemyManager::OnLevelBegun(LevelLoadData level)
{
	const std::vector<LevelConfigData>& levels = level.Source == LevelSource::ArcadeLevels ? ConfigFile->GetLevelsConfigData() : ConfigFile->GetMapMakerLevelsConfigData();
	const LevelConfigData& startedLevel = levels[level.LevelIndex];
	NumEnemySpawnersThisLevel = startedLevel.MonsterSpawners.size();
	u32 monstersThisLevel = 0;
	u32 i = 0;
	for (const MonsterSpawnerData& spawnerData : startedLevel.MonsterSpawners)
	{
		EnemySpawner& spawner = EnemySpawnerPool[i++];
		spawner.TileCoords = spawnerData.TilePosition;
		spawner.NumberOfEnemiesLeftToSpawn = spawnerData.NumMonsters;
		spawner.Timer = 0;
		spawner.State = EnemySpawnerState::Idle;
		spawner.bFlashState = true;
		monstersThisLevel += spawnerData.NumMonsters;
	}
	MaxEnemiesThisLevel = monstersThisLevel;
	NumEnemiesSpawned = 0;
}

void EnemyManager::OnResetAfterDeath(LevelLoadData level)
{
	for (int i = 0; i < NumEnemiesSpawned; i++)
	{
		Enemy& enemy = EnemyPool[i];
		if (enemy.bActive)
		{
			enemy.OriginSpawner->NumberOfEnemiesLeftToSpawn++;
			enemy.bActive = false;
		}
	}
	for (int i = 0; i < NumEnemySpawnersThisLevel; i++)
	{
		EnemySpawnerPool[i].State = EnemySpawnerState::Idle;
		EnemySpawnerPool[i].Timer = 0.0f;
		EnemySpawnerPool[i].bFlashState = true;
	}

	NumEnemiesSpawned = 0;
}

void EnemyManager::PopulateAnimationTables()
{
	AnimationAssetManager->MakeAnimationRectFramesFromName("NormalEnemy_RunRight", NormalEnemyAnimationTable[0][(i32)MovementDirection::Right]);
	AnimationAssetManager->MakeAnimationRectFramesFromName("NormalEnemy_RunDown",  NormalEnemyAnimationTable[0][(i32)MovementDirection::Down]);
	AnimationAssetManager->MakeAnimationRectFramesFromName("NormalEnemy_RunLeft",  NormalEnemyAnimationTable[0][(i32)MovementDirection::Left]);
	AnimationAssetManager->MakeAnimationRectFramesFromName("NormalEnemy_RunUp",    NormalEnemyAnimationTable[0][(i32)MovementDirection::Up]);

	AnimationAssetManager->MakeAnimationRectFramesFromName("NormalEnemy_RunRight_Digging", NormalEnemyAnimationTable[1][(i32)MovementDirection::Right]);
	AnimationAssetManager->MakeAnimationRectFramesFromName("NormalEnemy_RunDown_Digging",  NormalEnemyAnimationTable[1][(i32)MovementDirection::Down]);
	AnimationAssetManager->MakeAnimationRectFramesFromName("NormalEnemy_RunLeft_Digging",  NormalEnemyAnimationTable[1][(i32)MovementDirection::Left]);
	AnimationAssetManager->MakeAnimationRectFramesFromName("NormalEnemy_RunUp_Digging",    NormalEnemyAnimationTable[1][(i32)MovementDirection::Up]);
}

void EnemyManager::UpdateSingleSpawner(float deltaTime, EnemySpawner& spawner)
{
	spawner.Timer += deltaTime;
	switch (spawner.State)
	{
	case EnemySpawnerState::Idle:
		if (spawner.Timer >= SpawnEnemyInterval)
		{
			spawner.Timer = 0;
			spawner.State = EnemySpawnerState::Flashing;
			spawner.FlashCounter = 0;
			spawner.bFlashState = true;
		}
		break;
	case EnemySpawnerState::Flashing:
		if (spawner.Timer >= SpawnEnemyFlashInterval)
		{
			spawner.Timer = 0.0f;
			spawner.bFlashState = !spawner.bFlashState;
			++spawner.FlashCounter;
			if (spawner.FlashCounter >= FlashesBeforeSpawn)
			{
				SpawnEnemy(spawner);
				spawner.State = EnemySpawnerState::Idle;
			}
		}
		break;
	}
}

void EnemyManager::SpawnEnemy(EnemySpawner& spawner)
{
	--spawner.NumberOfEnemiesLeftToSpawn;
	Enemy& spawned = EnemyPool[NumEnemiesSpawned++];
	spawned.bActive = true;
	spawned.OriginSpawner = &spawner;
	spawned.Type = EnemyType::Normal;
	spawned.Pos = { spawner.TileCoords.x * (float)BackgroundTileSize, spawner.TileCoords.y * (float)BackgroundTileSize };
}
