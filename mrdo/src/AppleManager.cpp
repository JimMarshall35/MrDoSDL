#include "AppleManager.h"
#include "IConfigFile.h"
#include "Event.h"
#include <iostream>
#include <cassert>
#include "IAnimationAssetManager.h"
#include "Character.h"
#include "CollisionHelpers.h"
#include "TiledWorld.h"
#include "EnemyManager.h"
#include "GameFramework.h"
#include "GameFrameworkMessages.h"

AppleManager::AppleManager(
	const std::shared_ptr<IAnimationAssetManager>& assetManager,
	const std::shared_ptr<IConfigFile>& configFile,
	const std::shared_ptr<TiledWorld>& tiledWorld,
	Event<LevelLoadData>& onNewLevelStarted,
	EnemyManager* enemyManager
)
	:CachedConfig(configFile),
	AnimationAssetManager(assetManager),
	ApplePoolSize(configFile->GetUIntValue("ApplePoolSize")),
	ThisLevelNumApplesAtStart(0),
	ApplePool(std::make_unique<Apple[]>(ApplePoolSize)),
	LOnNewLevelStarted(this),
	CachedSpriteDims(vec2{ (float)configFile->GetAnimationsConfigData().TileSize, (float)configFile->GetAnimationsConfigData().TileSize }),
	PushedAppleStack(std::make_unique<Apple*[]>(ApplePoolSize)),
	CachedTiledWorld(tiledWorld),
	AppleSlideSpeed(configFile->GetFloatValue("AppleSlideSpeed")),
	AppleFallSpeed(configFile->GetFloatValue("AppleFallSpeed")),
	AppleWobbleTime(configFile->GetFloatValue("AppleWobbleTime")),
	AppleSplitTime(configFile->GetFloatValue("AppleSplitTime")),
	CachedEnemyManager(enemyManager)
{
	onNewLevelStarted += &LOnNewLevelStarted;
	CachedBackgroundTileSize = configFile->GetBackgroundConfigData().TileSize;
	assetManager->MakeAnimationRectFramesFromName("Apple_Wobble", WobbleAnimation);
	assetManager->MakeAnimationRectFramesFromName("Apple_Split_LeftHalf", LeftHalfSplitAnimation);
	assetManager->MakeAnimationRectFramesFromName("Apple_Split_RightHalf", RightHalfSplitAnimation);
	assert(LeftHalfSplitAnimation.size() == RightHalfSplitAnimation.size());
	assert(LeftHalfSplitAnimation.size() == 3);
	assert(WobbleAnimation.size() == 3);
}

void AppleManager::Update(float deltaT)
{
	for (int i = 0; i < ThisLevelNumApplesAtStart; i++)
	{
		Apple& apple = ApplePool[i];
		if (apple.State != AppleState::Inactive)
		{
			UpdateSingleApple(deltaT, apple);
		}
	}
}

void AppleManager::Draw(SDL_Surface* windowSurface, float scale) const
{
	SDL_Surface* surface = AnimationAssetManager->GetAnimationsSpriteSheetSurface();
	for (int i = 0; i < ThisLevelNumApplesAtStart; i++)
	{
		const Apple& apple = ApplePool[i];
		if (apple.State != AppleState::Inactive)
		{
			switch (apple.State)
			{
			case AppleState::Falling:
			case AppleState::Settled:
				{
					SDL_Rect dst;
					dst.w = CachedBackgroundTileSize * scale;
					dst.h = CachedBackgroundTileSize * scale;
					dst.x = (float)apple.Position.x * scale;
					dst.y = (float)apple.Position.y * scale;

					const SDL_Rect& rect = WobbleAnimation[1];
					SDL_BlitSurfaceScaled(surface, &rect, windowSurface, &dst);
				}
				break;
			case AppleState::Sliding:
			case AppleState::Wobbling:
				{
					SDL_Rect dst;
					dst.w = CachedBackgroundTileSize * scale;
					dst.h = CachedBackgroundTileSize * scale;
					dst.x = (float)apple.Position.x * scale;
					dst.y = (float)apple.Position.y * scale;

					const SDL_Rect& rect = WobbleAnimation[apple.OnAnimationFrame];
					SDL_BlitSurfaceScaled(surface, &rect, windowSurface, &dst);
				}
				break;
			case AppleState::Splitting:
				{
					SDL_Rect leftHalfDst;
					SDL_Rect rightHalfDst;
					leftHalfDst.x = (float)apple.Position.x * scale - ((float)CachedSpriteDims.x / 2.0f) * scale;
					leftHalfDst.y = apple.Position.y * scale;
					leftHalfDst.w = CachedSpriteDims.x * scale;
					leftHalfDst.h = CachedSpriteDims.y * scale;

					rightHalfDst.x = (float)apple.Position.x * scale + ((float)CachedSpriteDims.x / 2.0f) * scale;
					rightHalfDst.y = apple.Position.y * scale;
					rightHalfDst.w = CachedSpriteDims.x * scale;
					rightHalfDst.h = CachedSpriteDims.y * scale;
				
					const SDL_Rect& lRect = LeftHalfSplitAnimation[apple.OnAnimationFrame];
					SDL_BlitSurfaceScaled(surface, &lRect, windowSurface, &leftHalfDst);
					const SDL_Rect& rRect = RightHalfSplitAnimation[apple.OnAnimationFrame];
					SDL_BlitSurfaceScaled(surface, &rRect, windowSurface, &rightHalfDst);
				}
				break;
			}
			
		}
	}
}

void AppleManager::SetCharacter(Character* character)
{
	CharacterRef = character;
}

AppleManager::Apple* AppleManager::FindActiveAppleByPredicate(std::function<bool(const Apple&)> predicate)
{
	for (int i = 0; i < ThisLevelNumApplesAtStart; i++)
	{
		// just iterate through all the apples even non active ones.
		// Could maintain link field to next active apple to minimise iterations 
		// but I don't think its worth it given anticpiated number of apples.
		// Also if I did this it would still have to initially iterate through all of them
		const Apple& apple = ApplePool[i];
		if (apple.State != AppleState::Inactive)
		{
			if (predicate(apple))
			{
				return &ApplePool[i];
			}
		}
	}
	return nullptr;
}

void AppleManager::PopulateEnemyCollisionRelationships(Apple* apple)
{
	NumCollidingEnemies = 0;
	CachedEnemyManager->IterateActiveEnemies([this, apple](Enemy& enemy) {
		if (enemy.bActive && !enemy.bCrushed)
		{
			CollidingCellRelationship result = GetCollisionRelationshipBase(apple, enemy.Pos, CachedSpriteDims);

			
			switch (result)
			{
			case CollidingCellRelationship::Undefined:
			case CollidingCellRelationship::NotColliding:
				break;
			default:
				EnemyCollisionRelationships[NumCollidingEnemies++] = { result, &enemy };
				break;
			}
		}
	});
}

bool AppleManager::ResolveEnemyCollisions(Apple* apple)
{
	CachedEnemyManager->IterateActiveEnemies([this](Enemy& enemy) {
		CachedEnemyManager->SetEnemyPushingState(&enemy, false);
	});
	bool pushed = false;
	for (int i = 0; i < NumCollidingEnemies; i++)
	{
		SettledAppleEnemyCollision& col = EnemyCollisionRelationships[i];
		switch (col.CellRelationship)
		{
		case CollidingCellRelationship::Left:
			// enemy is approaching from the left therefore pushing the apple - resolve collision
			apple->Position.x = col.CollidedEnemy->Pos.x + CachedSpriteDims.x;
			pushed = true;
			break;
		case CollidingCellRelationship::Right:
			// enemy is approaching from the right therefore pushing the apple - resolve collision
			apple->Position.x = col.CollidedEnemy->Pos.x - CachedSpriteDims.x;
			pushed = true;
			break;
		case CollidingCellRelationship::Above:
			col.CollidedEnemy->Pos.y = apple->Position.y - CachedSpriteDims.y;
			break;
		default:
			break;
		}
		if (pushed)
		{
			CachedEnemyManager->SetEnemyPushingState(col.CollidedEnemy, true);
		}
	}
	return pushed;
}


void AppleManager::UpdateSingleApple(float deltaT, Apple& apple)
{
	switch (apple.State)
	{
	case AppleState::Settled:
		{
			bool pushing = false;
			vec2 characterPos = CharacterRef->GetPosition();
			switch(GetCollisionRelationshipWithMrDo(&apple))
			{
			case CollidingCellRelationship::Left:
				// mr do is approaching from the left therefore pushing the apple - resolve collision
				apple.Position.x = characterPos.x + CachedSpriteDims.x;
				pushing = true;
				break;
			case CollidingCellRelationship::Right:
				// mr do is approaching from the right therefore pushing the apple - resolve collision
				apple.Position.x = characterPos.x - CachedSpriteDims.x;
				pushing = true;
				break;
			case CollidingCellRelationship::Undefined:
			case CollidingCellRelationship::NotColliding:
				if (IsCellBelowEmpty(&apple) && !IsMrDoBelow(&apple) && !IsAppleBelow(&apple))
				{
					apple.State = AppleState::Wobbling;
				}
				break;
			}
			NumCollidingEnemies = 0;
			PopulateEnemyCollisionRelationships(&apple);
			bool pushedByEnemy = ResolveEnemyCollisions(&apple);
			

			if (pushing || pushedByEnemy)
			{
				PushedAppleStackSize = 0;
				PushedAppleStack[PushedAppleStackSize++] = &apple;

				RecursivelyPushApples(apple);
				ClampPushedApples();

				Apple* lastApple = PushedAppleStack[PushedAppleStackSize - 1];

				if (IsCellBelowEmpty(lastApple) && !IsAppleBelow(lastApple))
				{
					lastApple->State = AppleState::Sliding;
					float lastAppleCenterX = lastApple->Position.x + CachedSpriteDims.x / 2.0f;
					int cellBelowCoordsX = lastAppleCenterX / CachedBackgroundTileSize;
					lastApple->SlideDestination = cellBelowCoordsX * CachedBackgroundTileSize;
				}
			}
		}
		break;
	case AppleState::Falling:
		// possible transiitons: Settled, Spliiting
		{
			CollidingCellRelationship collisionWithDo = GetCollisionRelationshipWithMrDo(&apple);
			if (!apple.CrushedCharacter && collisionWithDo == CollidingCellRelationship::Below)
			{
				apple.CrushedCharacter = CharacterRef;
				CharacterRef->Crush();
			}

			CachedEnemyManager->IterateActiveEnemies([this, &apple](Enemy& enemy) {
				if (enemy.bCrushed)
				{
					return;
				}
				float dims = CachedSpriteDims.x;
				if (CollisionHelpers::AABBCollision(
					enemy.Pos + vec2{ A_SMALL_NUMBER / 2.0f, A_SMALL_NUMBER / 2.0f },
					apple.Position + vec2{ A_SMALL_NUMBER / 2.0f, A_SMALL_NUMBER / 2.0f },
					{ dims - A_SMALL_NUMBER, dims - A_SMALL_NUMBER },
					{ dims - A_SMALL_NUMBER, dims - A_SMALL_NUMBER }))
				{
					apple.EnemyBuffer[apple.EnemyBufferCurrentSize++] = &enemy;
					CachedEnemyManager->CrushEnemy(&enemy);
				}
			});

			float fallAmountThisFrame = 1.0f * AppleFallSpeed * deltaT;
			apple.Position.y += fallAmountThisFrame;
			apple.DistanceFallen += fallAmountThisFrame;

			if (apple.CrushedCharacter)
			{
				vec2 position = apple.CrushedCharacter->GetPosition();
				apple.CrushedCharacter->SetPosition({ position.x, apple.Position.y + CachedSpriteDims.y / 2.0f });
			}
			for (int i = 0; i < apple.EnemyBufferCurrentSize; i++)
			{
				Enemy* enemy = apple.EnemyBuffer[i];
				enemy->Pos.y = apple.Position.y + CachedSpriteDims.y / 2.0f;
			}

			if (!IsCellDirectlyBelowEmpty(&apple))
			{
				if (apple.DistanceFallen <= CachedBackgroundTileSize + CachedBackgroundTileSize * 0.5f)
				{
					vec2 appleCenter = apple.Position + vec2{ CachedSpriteDims.x / 2.0f, CachedSpriteDims.y / 2.0f };
					ivec2 appleCoords = { appleCenter.x / CachedBackgroundTileSize, appleCenter.y / CachedBackgroundTileSize };
					apple.Position = { appleCoords.x * CachedBackgroundTileSize, appleCoords.y * CachedBackgroundTileSize };
					apple.DistanceFallen = 0.0f;
					apple.State = AppleState::Settled;
					if (apple.EnemyBufferCurrentSize > 0)
					{
						KillAppleCrushedEnemies(apple);
					}
				}
				else
				{
					apple.AnimationTimer = 0.0f;
					apple.OnAnimationFrame = 0.0f;
					apple.State = AppleState::Splitting;
				}
			}
		}
		break;
	case AppleState::Splitting:
		// possible transition: None. Apple becomes inactive after animation
		{
			apple.AnimationTimer += deltaT;
			apple.OnAnimationFrame = (int)((apple.AnimationTimer / AppleSplitTime) * LeftHalfSplitAnimation.size());
			if (apple.AnimationTimer >= AppleSplitTime)
			{
				apple.State = AppleState::Inactive;
				if (apple.CrushedCharacter)
				{
					apple.CrushedCharacter->Kill(CharacterDeathReason::CrushedByApple);
					apple.CrushedCharacter = nullptr;
				}
				if (apple.EnemyBufferCurrentSize > 0)
				{
					KillAppleCrushedEnemies(apple);
				}
			}
		}
		break;
	case AppleState::Wobbling:
		// possible transition: falling
		{
			static const u32 wobblePatternSize = 5;
			static const u32 wobblePattern[5] = { 1, 0, 1, 2, 1 };
			apple.AnimationTimer += deltaT;
			apple.OnAnimationFrame = wobblePattern[(int)((apple.AnimationTimer / AppleWobbleTime) * wobblePatternSize)];
			if (apple.AnimationTimer >= AppleWobbleTime)
			{
				apple.AnimationTimer = 0;
				apple.DistanceFallen = 0.0f;
				apple.State = AppleState::Falling;
			}
		}
		break;
	case AppleState::Sliding:
		// possible transition: falling
		{
			CachedEnemyManager->IterateActiveEnemies([this, &apple](Enemy& enemy) {
				if (enemy.bCrushed)
				{
					return;
				}
				float dims = CachedSpriteDims.x;
				if (CollisionHelpers::AABBCollision(
					enemy.Pos + vec2{ A_SMALL_NUMBER / 2.0f, A_SMALL_NUMBER / 2.0f },
					apple.Position + vec2{ A_SMALL_NUMBER / 2.0f, A_SMALL_NUMBER / 2.0f },
					{ dims - A_SMALL_NUMBER, dims - A_SMALL_NUMBER },
					{ dims - A_SMALL_NUMBER, dims - A_SMALL_NUMBER }))
				{
					apple.EnemyBuffer[apple.EnemyBufferCurrentSize++] = &enemy;
					CachedEnemyManager->CrushEnemy(&enemy);
				}
			});
			float movmentdir = apple.SlideDestination - apple.Position.x > 0 ? 1.0f : -1.0f;
			if (movmentdir > 0)
			{
				apple.OnAnimationFrame = 0;
			}
			else
			{
				apple.OnAnimationFrame = 2;
			}
			apple.Position.x += movmentdir * AppleSlideSpeed * deltaT;
			float newMovmentdir = apple.SlideDestination - apple.Position.x > 0 ? 1.0f : -1.0f;
			if (newMovmentdir != movmentdir)
			{
				apple.Position.x = apple.SlideDestination;
				apple.OnAnimationFrame = 0;
				apple.AnimationTimer = 0;
				apple.DistanceFallen = 0.0f;
				apple.State = AppleState::Falling;
			}
		}
		break;
	}
}

void AppleManager::KillAppleCrushedEnemies(Apple& apple)
{
	EnemyDeath d;
	d.NumberKilledTotal = 0;
	d.NumberSignificantKilled = 0;
	d.Reason = EnemyDeathReason::Apple;
	for (int i = 0; i < apple.EnemyBufferCurrentSize; i++)
	{
		if (apple.EnemyBuffer[i]->bActive)
		{
			if (IsSignificantEnemyType(apple.EnemyBuffer[i]->Type))
			{
				d.NumberSignificantKilled++;
			}
			d.NumberKilledTotal++;
			CachedEnemyManager->KillEnemy(apple.EnemyBuffer[i]);
		}
	}
	GameFramework::SendFrameworkMessage<EnemyDeath>(d);
}

void AppleManager::RecursivelyPushApples(Apple& apple)
{
	for (int i = 0; i < ThisLevelNumApplesAtStart; i++)
	{
		Apple& otherApple = ApplePool[i];
		if ((apple.State == AppleState::Settled) && (&otherApple != &apple) && (otherApple.State == AppleState::Settled) && (otherApple.Position.y == apple.Position.y))
		{
			if (CollisionHelpers::AABBCollision(
				apple.Position,
				otherApple.Position,
				CachedSpriteDims,
				CachedSpriteDims))
			{
				if (apple.Position.x < otherApple.Position.x)
				{
					otherApple.Position.x = apple.Position.x + CachedSpriteDims.x + A_SMALL_NUMBER;
				}
				else
				{
					otherApple.Position.x = apple.Position.x - CachedSpriteDims.x - A_SMALL_NUMBER;
				}
				PushedAppleStack[PushedAppleStackSize++] = &otherApple;
				RecursivelyPushApples(otherApple);
			}

			CachedEnemyManager->IterateActiveEnemies([this, &apple](Enemy& enemy) {
				if (enemy.bCrushed)
				{
					return;
				}
				float dims = CachedSpriteDims.x;
				if (CollisionHelpers::AABBCollision(
					enemy.Pos + vec2{ A_SMALL_NUMBER / 2.0f, A_SMALL_NUMBER / 2.0f },
					apple.Position + vec2{ A_SMALL_NUMBER / 2.0f, A_SMALL_NUMBER / 2.0f },
					{ dims - A_SMALL_NUMBER, dims - A_SMALL_NUMBER },
					{ dims - A_SMALL_NUMBER, dims - A_SMALL_NUMBER }))
				{
					if (apple.Position.x < enemy.Pos.x)
					{
						enemy.Pos.x = apple.Position.x + CachedSpriteDims.x + A_SMALL_NUMBER;
					}
					else
					{
						enemy.Pos.x = apple.Position.x - CachedSpriteDims.x - A_SMALL_NUMBER;
					}
				}
			});
		}
	}
}

void AppleManager::ClampPushedApples(bool pushedByEnemy)
{
	assert(PushedAppleStackSize);
	vec2 characterPos = CharacterRef->GetPosition();
	Apple* endApple = PushedAppleStack[PushedAppleStackSize - 1];
	float tileMaxX = (CachedLevelSize.x - 1) * CachedBackgroundTileSize;

	CollidingCellRelationship relationship = GetCollisionRelationshipWithMrDo(endApple);
	// does the end epple in the stack need to be clamped
	if (endApple->Position.x > tileMaxX || endApple->Position.x < 0 || relationship != CollidingCellRelationship::NotColliding)
	{
		// clamp the last apple being pushed to the edges of the screen
		if (endApple->Position.x > tileMaxX)
		{
			endApple->Position.x = tileMaxX;
		}
		else if (endApple->Position.x < 0)
		{
			endApple->Position.x = 0;
		}

		
		switch (relationship)
		{
		case CollidingCellRelationship::NotColliding:
			break;
		default:
			if (characterPos.x < endApple->Position.x)
			{
				endApple->Position.x = characterPos.x + CachedSpriteDims.x;
			}
			else
			{
				endApple->Position.x = characterPos.x - CachedSpriteDims.x;
			}
			break;
		}

		// propagate collision resolution to all the other apples
		for (int i = PushedAppleStackSize - 2; i >= 0; i--)
		{
			Apple* apple = PushedAppleStack[i];
			Apple* previousApple = PushedAppleStack[i + 1];
			
			if (apple->Position.x < previousApple->Position.x)
			{
				apple->Position.x = previousApple->Position.x - CachedSpriteDims.x;
			}
			else
			{
				apple->Position.x = previousApple->Position.x + CachedSpriteDims.x;
			}				
		}

		// propagate collision resolution to mr do
		Apple* firstApple = PushedAppleStack[0];
		if (characterPos.x < firstApple->Position.x)
		{
			CharacterRef->SetPosition({ firstApple->Position.x - CachedSpriteDims.x, characterPos.y });
		}
		else
		{
			CharacterRef->SetPosition({ firstApple->Position.x + CachedSpriteDims.x, characterPos.y });
		}

		for (int i = 0; i < NumCollidingEnemies; i++)
		{
			const SettledAppleEnemyCollision& collision = EnemyCollisionRelationships[i];

			if (collision.CollidedEnemy->Pos.x < firstApple->Position.x)
			{
				collision.CollidedEnemy->Pos.x = firstApple->Position.x - CachedSpriteDims.x;
				u8 cell = CachedTiledWorld->GetCellAtIndexValue(ivec2{ lroundf(collision.CollidedEnemy->Pos.x / (float)CachedBackgroundTileSize), lroundf((collision.CollidedEnemy->Pos.y + A_SMALL_NUMBER) / (float)CachedBackgroundTileSize) });
				u8 currentCell = CachedTiledWorld->GetCellAtIndexValue({ (int)collision.CollidedEnemy->CurrentCell.x, (int)collision.CollidedEnemy->CurrentCell.y });;
				if (cell & (1 << (int)TileWallDirectionBit::Right) || currentCell & (1 << (int)TileWallDirectionBit::Left))
				{
					collision.CollidedEnemy->Pos.x = lroundf(collision.CollidedEnemy->Pos.x / (float)CachedBackgroundTileSize) * (float)CachedBackgroundTileSize;
				}
			}
			else
			{
				collision.CollidedEnemy->Pos.x = firstApple->Position.x + CachedSpriteDims.x;
				u8 cell = CachedTiledWorld->GetCellAtIndexValue(ivec2{ lroundf(collision.CollidedEnemy->Pos.x / (float)CachedBackgroundTileSize), lroundf((collision.CollidedEnemy->Pos.y + A_SMALL_NUMBER) / (float)CachedBackgroundTileSize) });
				u8 currentCell = CachedTiledWorld->GetCellAtIndexValue({ (int)collision.CollidedEnemy->CurrentCell.x, (int)collision.CollidedEnemy->CurrentCell.y });;
				if (cell & (1 << (int)TileWallDirectionBit::Left) || currentCell & (1 << (int)TileWallDirectionBit::Right))
				{
					collision.CollidedEnemy->Pos.x = lroundf(collision.CollidedEnemy->Pos.x / (float)CachedBackgroundTileSize) * (float)CachedBackgroundTileSize;
				}
			}
		}
	}
}

void AppleManager::OnNewLevelStarted(LevelLoadData levelLoadData)
{
	const std::vector<LevelConfigData>& levels = levelLoadData.Source == LevelSource::ArcadeLevels ? CachedConfig->GetLevelsConfigData() : CachedConfig->GetMapMakerLevelsConfigData();
	const LevelConfigData& startedLevel = levels[levelLoadData.LevelIndex];
	ThisLevelNumApplesAtStart = startedLevel.Apples.size();
	assert(ThisLevelNumApplesAtStart <= ApplePoolSize);

	// get max number of monsters
	MaxMonstersThisLevel = 0;
	for (const MonsterSpawnerData& spawnerData : startedLevel.MonsterSpawners)
	{
		MaxMonstersThisLevel += spawnerData.NumMonsters;
	}

	if (!EnemyCollisionRelationships.get())
	{
		EnemyCollisionRelationships = std::make_unique<SettledAppleEnemyCollision[]>(MaxMonstersThisLevel);
	}
	else
	{
		EnemyCollisionRelationships.reset();
		EnemyCollisionRelationships = std::make_unique<SettledAppleEnemyCollision[]>(MaxMonstersThisLevel);
	}
	
	for (int i = 0; i < ThisLevelNumApplesAtStart; i++)
	{
		ApplePool[i].State = AppleState::Settled;
		ApplePool[i].Position = vec2{ (float)startedLevel.Apples[i].x * CachedBackgroundTileSize, (float)startedLevel.Apples[i].y * CachedBackgroundTileSize };
		ApplePool[i].AnimationTimer = 0.0f;
		ApplePool[i].OnAnimationFrame = 0;
		ApplePool[i].DistanceFallen = 0.0f;
		ApplePool[i].CrushedCharacter = nullptr;
		if (ApplePool[i].EnemyBuffer.get())
		{
			ApplePool[i].EnemyBuffer.reset();
		}
		ApplePool[i].EnemyBuffer = std::make_unique<Enemy*[]>(MaxMonstersThisLevel);
		ApplePool[i].EnemyBufferCurrentSize = 0;
	}
	CachedLevelSize.x = startedLevel.NumCols;
	CachedLevelSize.y = startedLevel.NumRows;

}

bool AppleManager::IsCellBelowEmpty(Apple* apple) const
{
	float lastAppleCenterX = apple->Position.x + CachedSpriteDims.x / 2.0f;
	float lastAppleCenterY = apple->Position.y + CachedSpriteDims.y / 2.0f;
	float cellBelowY = lastAppleCenterY + CachedBackgroundTileSize;
	int cellBelowCoordsX = lastAppleCenterX / CachedBackgroundTileSize;
	int cellBelowCorrdsY = cellBelowY / CachedBackgroundTileSize;
	u8 cell = CachedTiledWorld->GetCellAtIndexValue({ cellBelowCoordsX, cellBelowCorrdsY });
	return (cell & (1 << (u8)TileWallDirectionBit::Center)) == 0;
}

bool AppleManager::IsCellDirectlyBelowEmpty(Apple* apple) const
{
	float lastAppleCenterX = apple->Position.x + CachedSpriteDims.x / 2.0f;
	float lastAppleCenterY = apple->Position.y + CachedSpriteDims.y / 2.0f;
	float cellBelowY = lastAppleCenterY + CachedBackgroundTileSize / 2.0f + A_SMALL_NUMBER;
	int cellBelowCoordsX = lastAppleCenterX / CachedBackgroundTileSize;
	int cellBelowCorrdsY = cellBelowY / CachedBackgroundTileSize;
	u8 cell = CachedTiledWorld->GetCellAtIndexValue({ cellBelowCoordsX, cellBelowCorrdsY });
	return (cell & (1 << (u8)TileWallDirectionBit::Center)) == 0;
}

bool AppleManager::IsMrDoBelow(Apple* apple) const
{
	vec2 cellBelowPos = GetCellBelowPos(apple);
	vec2 characterPos = CharacterRef->GetPosition() + vec2{A_SMALL_NUMBER/2.0f, A_SMALL_NUMBER/2.0f};
	if (CollisionHelpers::AABBCollision(
		cellBelowPos,
		characterPos,
		{ CachedBackgroundTileSize, CachedBackgroundTileSize },
		CachedSpriteDims - vec2{A_SMALL_NUMBER, A_SMALL_NUMBER}))
	{
		return true;
	}
	return false;
}

bool AppleManager::IsAppleBelow(Apple* apple) const
{
	vec2 cellBelowPos = GetCellBelowPos(apple);
	cellBelowPos += {A_SMALL_NUMBER / 2.0f, A_SMALL_NUMBER / 2.0f};
	for (int i = 0; i < ThisLevelNumApplesAtStart; i++)
	{
		const Apple& otherApple = ApplePool[i];
		if ((otherApple.State == AppleState::Settled) && (&otherApple != apple))
		{
			if (CollisionHelpers::AABBCollision(
				cellBelowPos,
				otherApple.Position,
				{ CachedBackgroundTileSize - A_SMALL_NUMBER, CachedBackgroundTileSize - A_SMALL_NUMBER },
				{ CachedBackgroundTileSize, CachedBackgroundTileSize }))
			{
				return true;
			}
		}
	}
	return false;
}

vec2 AppleManager::GetCellBelowPos(Apple* apple) const
{
	vec2 appleCenter = apple->Position + vec2{ CachedSpriteDims.x / 2.0f, CachedSpriteDims.y / 2.0f };
	ivec2 appleCoords = { appleCenter.x / CachedBackgroundTileSize, appleCenter.y / CachedBackgroundTileSize };
	vec2 rVal = { appleCoords.x * CachedBackgroundTileSize, (appleCoords.y + 1)* CachedBackgroundTileSize };
	return rVal;
}

CollidingCellRelationship AppleManager::GetCollisionRelationshipBase(Apple* apple, const vec2& pos, const vec2& dims) const
{
	vec2 characterPos = CharacterRef->GetPosition();
	if (CollisionHelpers::AABBCollision(
		apple->Position + vec2{ A_SMALL_NUMBER / 2.0f, A_SMALL_NUMBER / 2.0f },
		pos,
		CachedSpriteDims - vec2{ A_SMALL_NUMBER, A_SMALL_NUMBER }, // take a small number from either side of our dims so we don't collide with apples to the left and right of us
		dims))
	{

		if ((apple->Position.x > pos.x) && (round(apple->Position.y) == round(pos.y)))
		{
			// mr do is approaching from the left
			return CollidingCellRelationship::Left;
		}
		else if ((apple->Position.x < pos.x) && (round(apple->Position.y) == round(pos.y)))
		{
			// mr do is approaching from the right
			return CollidingCellRelationship::Right;
		}
		if ((apple->Position.y > pos.y) && (round(apple->Position.x) == round(pos.x)))
		{
			// mr do is approaching from the top 
			return CollidingCellRelationship::Above;
		}
		else if ((apple->Position.y < pos.y) && (round(apple->Position.x) == round(pos.x)))
		{
			// mr do is approaching from below
			return CollidingCellRelationship::Below;
		}
		return CollidingCellRelationship::Undefined;
	}
	else
	{
		return CollidingCellRelationship::NotColliding;
	}

}

CollidingCellRelationship AppleManager::GetCollisionRelationshipWithMrDo(Apple* apple) const
{
	return GetCollisionRelationshipBase(apple, CharacterRef->GetPosition(), CachedSpriteDims);
}
