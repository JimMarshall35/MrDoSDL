#include "EnemyManager.h"
#include "IAnimationAssetManager.h"
#include "IConfigFile.h"
#include "Character.h"
#include "PathFinding.h"
#include "TiledWorld.h"
#include "MovementHelpers.h"
#include "CollisionHelpers.h"
#include <cassert>

std::vector<SDL_Rect> EnemyManager::NormalEnemyAnimationTable[2][4];
SDL_Rect EnemyManager::SpawnerTileSprite;


EnemyManager::EnemyManager(
	const std::shared_ptr<IConfigFile>& configFile,
	const std::shared_ptr<IAnimationAssetManager>& animationAssetManager,
	TiledWorld* tiledWorld,
	Event<LevelLoadData>& onLevelLoaded,
	Event<LevelLoadData>& onResetAfterDeath,
	Character* character)
	:LOnLevelBegun(this),
	LOnResetAfterDeath(this),
	ConfigFile(configFile),
	AnimationAssetManager(animationAssetManager),
	CachedTiledWorld(tiledWorld),
	EnemySpawnerPoolSize(configFile->GetUIntValue("MonsterSpawnerPoolSize")),
	EnemyPoolSize(configFile->GetUIntValue("MonsterPoolSize")),
	EnemySpawnerPool(std::make_unique<EnemySpawner[]>(EnemySpawnerPoolSize)),
	EnemyPool(std::make_unique<Enemy[]>(EnemyPoolSize)),
	BackgroundTileSize(configFile->GetBackgroundConfigData().TileSize),
	SpawnEnemyInterval(configFile->GetFloatValue("SpawnEnemyInterval")),
	SpawnEnemyFlashInterval(configFile->GetFloatValue("SpawnEnemyFlashInterval")),
	FlashesBeforeSpawn(configFile->GetUIntValue("FlashesBeforeSpawn")),
	CachedCharacter(character),
	PathBufferSize(configFile->GetUIntValue("EnemyPathBufferSize")),
	EnemySpeed(configFile->GetFloatValue("EnemySpeed"))
{
	CachedTileSize = ConfigFile->GetAnimationsConfigData().TileSize;
	onLevelLoaded += &LOnLevelBegun;
	onResetAfterDeath += &LOnResetAfterDeath;
	AnimationAssetManager->MakeSingleSpriteRectFrame("MonsterSpawner", SpawnerTileSprite);
	PopulateAnimationTables();
	InitialiseEnemyPool();
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
	for (int i = 0; i < NumEnemiesSpawned; i++)
	{
		Enemy& enemy = EnemyPool[i];
		if (enemy.bActive)
		{
			UpdateSingleEnemy(deltaTime, enemy);
		}
	}
}

void EnemyManager::Draw(SDL_Surface* windowSurface, float scale) const
{
	SDL_Rect dst;
	SDL_Surface* srcSurface = AnimationAssetManager->GetAnimationsSpriteSheetSurface();
	for (int i = 0; i < NumEnemySpawnersThisLevel; i++)
	{
		const EnemySpawner& spawner = EnemySpawnerPool[i];
		if (spawner.NumberOfEnemiesLeftToSpawn > 0)
		{
			
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

	for (int i = 0; i < NumEnemiesSpawned; i++)
	{
		if (EnemyPool[i].bActive)
		{
			Enemy& enemy = EnemyPool[i];
			dst.w = BackgroundTileSize * scale;
			dst.h = BackgroundTileSize * scale;
			dst.x = enemy.Pos.x * scale;
			dst.y = enemy.Pos.y * scale;
			SDL_BlitSurfaceScaled(srcSurface, &enemy.EnemyAnimator.GetCurrentFrame(), windowSurface, &dst);
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
	PathFinding::ResizeNodeMap(startedLevel.NumCols, startedLevel.NumRows);
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
	const ivec2& characterTile = CachedCharacter->GetTile();
	
	spawned.CurrentCell = spawner.TileCoords;
	spawned.EnemyAnimator.bIsAnimating = true;
	spawned.EnemyAnimator.FPS = 4;

	SetNewPath(spawned, characterTile);
	SetEnemyDestinationWorldSpace(spawned);
	SetEnemyDirection(spawned);
}

void EnemyManager::UpdateSingleEnemy(float deltaTime, Enemy& enemy)
{
	//move towards next path node
	vec2 moveDirection = (enemy.Destination - enemy.Pos).Normalized();
	float* coordinateToChangePtr = nullptr;
	float changeDirection = 1.0f;
	switch (enemy.CurrentDirection)
	{
	case MovementDirection::Up:
		coordinateToChangePtr = &enemy.Pos.y;
		changeDirection = -1.0f;
		break;
	case MovementDirection::Right:
		coordinateToChangePtr = &enemy.Pos.x;
		break;
	case MovementDirection::Down:
		coordinateToChangePtr = &enemy.Pos.y;
		break;
	case MovementDirection::Left:
		coordinateToChangePtr = &enemy.Pos.x;
		changeDirection = -1.0f;
		break;
	}

	(*coordinateToChangePtr) += deltaTime * EnemySpeed * changeDirection;
	bool passedIntoNextCell = false;
	float overshootAmount = 0.0f;
	switch (enemy.CurrentDirection)
	{
	case MovementDirection::Up:
		passedIntoNextCell = enemy.Pos.y <= enemy.Destination.y;
		overshootAmount = enemy.Destination.y - enemy.Pos.y;
		break;
	case MovementDirection::Right:
		passedIntoNextCell = enemy.Pos.x >= enemy.Destination.x;
		overshootAmount = enemy.Pos.x - enemy.Destination.x;
		break;
	case MovementDirection::Down:
		passedIntoNextCell = enemy.Pos.y >= enemy.Destination.y;
		overshootAmount = enemy.Pos.y - enemy.Destination.y;

		break;
	case MovementDirection::Left:
		passedIntoNextCell = enemy.Pos.x <= enemy.Destination.x;
		overshootAmount = enemy.Destination.x - enemy.Pos.x;
		break;
	}
	assert(overshootAmount < (float)CachedTiledWorld->GetTileSize());
	if (passedIntoNextCell)
	{
		enemy.Pos = enemy.Destination;
		enemy.CurrentCell = enemy.PathBuffer[enemy.PathBufferDestinationIndex];

		if (--enemy.PathBufferDestinationIndex < 0)
		{
			const ivec2& characterTile = CachedCharacter->GetTile();
			SetNewPath(enemy, characterTile);
		}

		SetEnemyDestinationWorldSpace(enemy);
		SetEnemyDirection(enemy);

		// apply amount overshot
		vec2 newMovementVector = MovementHelpers::GetDirectionVector(enemy.CurrentDirection);
		enemy.Pos += newMovementVector * overshootAmount;
	}
	enemy.EnemyAnimator.CurrentAnimation = &NormalEnemyAnimationTable[enemy.bPushing][(u32)enemy.CurrentDirection];
	enemy.EnemyAnimator.Update(deltaTime / 1000.f);
	if (CollisionHelpers::AABBCollision(enemy.Pos, CachedCharacter->GetPosition(), { CachedTileSize ,CachedTileSize },  { CachedTileSize, CachedTileSize }))
	{
		CachedCharacter->Kill(CharacterDeathReason::KilledByMonster);
	}

}
void EnemyManager::SetEnemyDestinationWorldSpace(Enemy& enemy)
{
	const uvec2& dest = enemy.PathBuffer[enemy.PathBufferDestinationIndex];
	enemy.Destination = { dest.x * (float)BackgroundTileSize, dest.y * (float)BackgroundTileSize };
}

void EnemyManager::InitialiseEnemyPool()
{
	for (int i = 0; i < EnemyPoolSize; i++)
	{
		EnemyPool[i].PathBuffer = std::make_unique<uvec2[]>(PathBufferSize);
	}
}

void EnemyManager::SetNewPath(Enemy& enemy, const ivec2& newDestinationCell)
{
	PathFinding::DoAstar(
		enemy.CurrentCell,
		uvec2{ (u32)newDestinationCell.x, (u32)newDestinationCell.y },
		enemy.PathBuffer.get(),
		enemy.PathBufferCurrentSize,
		PathBufferSize,
		CachedTiledWorld
	);
	enemy.PathBufferDestinationIndex = enemy.PathBufferCurrentSize - 1;
}

void EnemyManager::SetEnemyDirection(Enemy& enemy)
{
	const uvec2& current = enemy.CurrentCell;
	const uvec2& destination = enemy.PathBuffer[enemy.PathBufferDestinationIndex];

	int dx = current.x - destination.x;
	int dy = current.y - destination.y;
	assert((dx >= -1) && (dx <= 1));
	assert((dy >= -1) && (dy <= 1));
	assert((dx == 0) || (dy == 0));

	if (dx == 1)
	{
		enemy.CurrentDirection = MovementDirection::Left;
	}
	else if (dx == -1)
	{
		enemy.CurrentDirection = MovementDirection::Right;
	}
	else if (dy == 1)
	{
		enemy.CurrentDirection = MovementDirection::Up;
	}
	else if (dy == -1)
	{
		enemy.CurrentDirection = MovementDirection::Down;
	}

}
