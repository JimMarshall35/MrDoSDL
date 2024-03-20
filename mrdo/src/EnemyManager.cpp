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
std::vector<SDL_Rect> EnemyManager::DiggerAnimationTable[4];
std::vector<SDL_Rect> EnemyManager::TransformingToDiggerAnimationTable[4];
std::vector<SDL_Rect> EnemyManager::ExtraMenAnimationTable[5];

SDL_Rect EnemyManager::SpawnerTileSprite;
SDL_Rect EnemyManager::CrushedMonsterSprite;
float EnemyManager::DeltaTime = 0.0f;

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
	EnemySpeed(configFile->GetFloatValue("EnemySpeed")),
	DigSpeedMultiplier(configFile->GetFloatValue("EnemyDigSpeedMultiplier")),
	PushSpeedMultiplier(configFile->GetFloatValue("EnemyPushSpeedMultiplier")),
	EnemyWaitTimeBeforeBecomeDigger(configFile->GetFloatValue("EnemyWaitTimeBeforeBecomeDigger")),
	MorphingEnemyFlashAnimationFPS(configFile->GetFloatValue("EnemyFlashAnimationFPS")),
	MorphingEnemyFlashTime(configFile->GetFloatValue("EnemyFlashTime")),
	EnemyCollider({configFile->GetVec2Value("EnemyColliderOffset")}),
	PlayerCollider({ configFile->GetVec2Value("PlayerColliderOffset") })

{
	CachedTileSize = ConfigFile->GetAnimationsConfigData().TileSize;
	onLevelLoaded += &LOnLevelBegun;
	onResetAfterDeath += &LOnResetAfterDeath;
	AnimationAssetManager->MakeSingleSpriteRectFrame("MonsterSpawner", SpawnerTileSprite);
	AnimationAssetManager->MakeSingleSpriteRectFrame("CrushedMonster", CrushedMonsterSprite),
	PopulateAnimationTables();
	InitialiseEnemyPool();
	UpdateNormalEnemyScriptFunction = EnemyScripting::FindExecutionToken("EnemyUpdate");
	FlashingEnemyScriptFunction = EnemyScripting::FindExecutionToken("FlashingEnemyUpdate");
	DiggerEnemyScriptFunction = EnemyScripting::FindExecutionToken("DiggerEnemyUpdate");
	ExtraManScriptFunction = EnemyScripting::FindExecutionToken("ExtraManEnemyUpdate");
	EnemyScripting::EnemyManager_ForthExposedMethodImplementations::Instance = this;

}

void EnemyManager::Update(float deltaTime)
{
	DeltaTime = deltaTime;
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
		if (enemy.bActive && !enemy.bCrushed)
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
			if (EnemyPool[i].bCrushed)
			{
				SDL_BlitSurfaceScaled(srcSurface, &CrushedMonsterSprite, windowSurface, &dst);
			}
			else
			{
				SDL_BlitSurfaceScaled(srcSurface, &enemy.EnemyAnimator.GetCurrentFrame(), windowSurface, &dst);
			}
			
		}
	}
}

void EnemyManager::IterateActiveEnemies(EnemyIterator iter) const
{
	for (int i = 0; i < NumEnemiesSpawned; i++)
	{
		if (EnemyPool[i].bActive)
		{
			iter(EnemyPool[i]);
		}
	}
}

int EnemyManager::GetActiveLevelMaxEnemies() const
{
	
	return MaxEnemiesThisLevel;
}

void EnemyManager::CrushEnemy(Enemy* enemy)
{
	assert(!enemy->bCrushed);
	enemy->bCrushed = true;
}

void EnemyManager::KillEnemy(Enemy* enemy)
{
	if (enemy->bActive)
	{
		enemy->bActive = false;
	}
	
	assert(enemy->OriginSpawner);
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

	AnimationAssetManager->MakeAnimationRectFramesFromName("FlashingEnemy_Right", TransformingToDiggerAnimationTable[(i32)MovementDirection::Right]);
	AnimationAssetManager->MakeAnimationRectFramesFromName("FlashingEnemy_Down", TransformingToDiggerAnimationTable[(i32)MovementDirection::Down]);
	AnimationAssetManager->MakeAnimationRectFramesFromName("FlashingEnemy_Left", TransformingToDiggerAnimationTable[(i32)MovementDirection::Left]);
	AnimationAssetManager->MakeAnimationRectFramesFromName("FlashingEnemy_Up", TransformingToDiggerAnimationTable[(i32)MovementDirection::Up]);

	AnimationAssetManager->MakeAnimationRectFramesFromName("RedDigger_Right", DiggerAnimationTable[(i32)MovementDirection::Right]);
	AnimationAssetManager->MakeAnimationRectFramesFromName("RedDigger_Down", DiggerAnimationTable[(i32)MovementDirection::Down]);
	AnimationAssetManager->MakeAnimationRectFramesFromName("RedDigger_Left", DiggerAnimationTable[(i32)MovementDirection::Left]);
	AnimationAssetManager->MakeAnimationRectFramesFromName("RedDigger_Up", DiggerAnimationTable[(i32)MovementDirection::Up]);

	AnimationAssetManager->MakeAnimationRectFramesFromName("ExtraMan_E", ExtraMenAnimationTable[0]);
	AnimationAssetManager->MakeAnimationRectFramesFromName("ExtraMan_X", ExtraMenAnimationTable[1]);
	AnimationAssetManager->MakeAnimationRectFramesFromName("ExtraMan_T", ExtraMenAnimationTable[2]);
	AnimationAssetManager->MakeAnimationRectFramesFromName("ExtraMan_R", ExtraMenAnimationTable[3]);
	AnimationAssetManager->MakeAnimationRectFramesFromName("ExtraMan_A", ExtraMenAnimationTable[4]);
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

Enemy& EnemyManager::SpawnEnemy(EnemySpawner& spawner, EnemyType type /* = EnemyType::Normal */)
{
	--spawner.NumberOfEnemiesLeftToSpawn;
	Enemy& spawned = EnemyPool[NumEnemiesSpawned++];
	spawned.bActive = true;
	spawned.OriginSpawner = &spawner;
	spawned.Type = type;
	spawned.Pos = { spawner.TileCoords.x * (float)BackgroundTileSize, spawner.TileCoords.y * (float)BackgroundTileSize };
	const ivec2& characterTile = CachedCharacter->GetTile();
	
	spawned.CurrentCell = spawner.TileCoords;
	spawned.EnemyAnimator.bIsAnimating = true;
	spawned.EnemyAnimator.FPS = 4;
	spawned.bCrushed = false;
	spawned.Timer = 0.0f;
	SetNewPath(spawned, characterTile);
	SetEnemyDestinationWorldSpace(spawned);
	SetEnemyDirection(spawned);
	return spawned;
}

void EnemyManager::UpdateSingleEnemy(float deltaTime, Enemy& enemy)
{
	EnemyType oldType = enemy.Type;
	Enemy* enemyPtr = &enemy;
	switch (enemy.Type)
	{
	case EnemyType::Normal:
		EnemyScripting::Push((Cell)enemyPtr);
		EnemyScripting::DoExecutionToken(UpdateNormalEnemyScriptFunction);
		break;
	case EnemyType::TurningIntoDigger:
		EnemyScripting::Push((Cell)enemyPtr);
		EnemyScripting::DoExecutionToken(FlashingEnemyScriptFunction);
		break;
	case EnemyType::Digger:
		EnemyScripting::Push((Cell)enemyPtr);
		EnemyScripting::DoExecutionToken(DiggerEnemyScriptFunction);
		break;
	case EnemyType::ExtraMan:
		EnemyScripting::Push((Cell)enemyPtr);
		EnemyScripting::DoExecutionToken(ExtraManScriptFunction);
	}
	EnemyType newType = enemy.Type;
}

void EnemyManager::SetEnemyPushingState(Enemy* enemy, bool newState)
{
	enemy->bPushing = newState;
}

void EnemyManager::SpawnExtraMan(const ivec2& location, int c)
{
	ExtraManDummySpawner.TileCoords = location;
	Enemy& e = SpawnEnemy(ExtraManDummySpawner, EnemyType::ExtraMan);
	e.EnemyAnimator.CurrentAnimation = &ExtraMenAnimationTable[c];
	e.ExtraManLetterIndex = c;
}


void EnemyManager::SetEnemyDestinationWorldSpace(Enemy& enemy)
{
	const ivec2& dest = enemy.PathBuffer[enemy.PathBufferDestinationIndex];
	enemy.Destination = { dest.x * (float)BackgroundTileSize, dest.y * (float)BackgroundTileSize };
}

bool EnemyManager::FollowPathBase(Enemy& enemy, float deltaTime, const PathFinishedCallback& onPathFinished, float speedMultiplier)
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

	(*coordinateToChangePtr) += deltaTime * EnemySpeed * changeDirection * speedMultiplier;

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
		//enemy.Timer = 0.0f;

		if (--enemy.PathBufferDestinationIndex < 0)
		{
			onPathFinished(enemy);
		}
		if (enemy.PathBufferDestinationIndex >= 0)
		{
			assert(enemy.PathBufferDestinationIndex >= 0);
			SetEnemyDestinationWorldSpace(enemy);
			SetEnemyDirection(enemy);
		}
		else
		{
			enemy.PathBufferDestinationIndex = 0;
		}
		

		// apply amount overshot
		vec2 newMovementVector = MovementHelpers::GetDirectionVector(enemy.CurrentDirection);
		enemy.Pos += newMovementVector * overshootAmount;
	}
	

	if (CollisionHelpers::AABBCollision(enemy.Pos + vec2{ A_SMALL_NUMBER / 2.0f + EnemyCollider.x / 2.0f, A_SMALL_NUMBER / 2.0f + EnemyCollider.y / 2.0f },
		CachedCharacter->GetPosition() + vec2{ A_SMALL_NUMBER / 2.0f + PlayerCollider.x / 2.0f, A_SMALL_NUMBER / 2.0f + PlayerCollider.y / 2.0f },
		{ CachedTileSize - A_SMALL_NUMBER  - EnemyCollider.x, CachedTileSize - A_SMALL_NUMBER - EnemyCollider.y },
		{ CachedTileSize - A_SMALL_NUMBER - PlayerCollider.x, CachedTileSize - A_SMALL_NUMBER - PlayerCollider.y }))
	{
		CachedCharacter->Kill(CharacterDeathReason::KilledByMonster);
	}
	return passedIntoNextCell;
}

void EnemyManager::InitialiseEnemyPool()
{
	for (int i = 0; i < EnemyPoolSize; i++)
	{
		EnemyPool[i].PathBuffer = std::make_unique<ivec2[]>(PathBufferSize);
	}
}

void EnemyManager::SetNewPath(Enemy& enemy, const ivec2& newDestinationCell)
{
	PathFinding::DoAStarNormalEnemy(
		enemy.CurrentCell,
		ivec2{ (u32)newDestinationCell.x, (u32)newDestinationCell.y },
		enemy.PathBuffer.get(),
		enemy.PathBufferCurrentSize,
		PathBufferSize,
		CachedTiledWorld
	);
	enemy.PathBufferDestinationIndex = enemy.PathBufferCurrentSize - 1;
	//assert(enemy.PathBufferDestinationIndex >= 0);
}

void EnemyManager::SetNewPathForDigger(Enemy& enemy, const ivec2& newDestinationCell, const ivec2& obstruction)
{
	PathFinding::DoDiggingEnemyAStar(
		enemy.CurrentCell,
		ivec2{ (u32)newDestinationCell.x, (u32)newDestinationCell.y },
		enemy.PathBuffer.get(),
		enemy.PathBufferCurrentSize,
		PathBufferSize,
		CachedTiledWorld,
		obstruction//enemy.PathBuffer[enemy.PathBufferDestinationIndex]
	);
	enemy.PathBufferDestinationIndex = enemy.PathBufferCurrentSize - 1;
	SetEnemyDestinationWorldSpace(enemy);
}

void EnemyManager::SetEnemyDirection(Enemy& enemy)
{
	const ivec2& current = enemy.CurrentCell;
	const ivec2& destination = enemy.PathBuffer[enemy.PathBufferDestinationIndex];

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
