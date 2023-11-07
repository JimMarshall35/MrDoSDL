#include "Character.h"
#include "IAnimationAssetManager.h"
#include "IConfigFile.h"
#include "TiledWorld.h"
#include "MovementHelpers.h"
#include <functional>
#include <cassert>
#include "Event.h"
#include "AppleManager.h"
#include "CollisionHelpers.h"
#include "GameFramework.h"
#include <cmath>

Character::Character(
	const std::shared_ptr<IAnimationAssetManager>& assetManager,
		const std::shared_ptr<IConfigFile>& configFile, 
		const std::shared_ptr<TiledWorld>& tiledWorld,
		Event<LevelLoadData>& onLevelLoaded,
		Event<LevelLoadData>& onResetAfterDeath,
		EnemyManager* enemyManager)
	:MyCrystalBall(this, assetManager.get(), configFile.get(), tiledWorld.get(), enemyManager),
	AnimationAssetManager(assetManager),
	ConfigFile(configFile),
	CachedTiledWorld(tiledWorld),
	LOnNewLevelStarted(this),
	CachedSpriteDims(vec2{ (float)configFile->GetAnimationsConfigData().TileSize, (float)configFile->GetAnimationsConfigData().TileSize }),
	LOnResetAfterDeath(this),
	DigSpeedMultiplier(configFile->GetFloatValue("CharacterDigSpeedMultiplier")),
	PushSpeedMultiplier(configFile->GetFloatValue("CharacterPushSpeedMultiplier"))
{
	onLevelLoaded += &LOnNewLevelStarted;
	onResetAfterDeath += &LOnResetAfterDeath;
	bIsMoving = false;
	bHasMoved = false;
	bBeingCrushed = false;
	bPreviousBarrierBetween = false;

	CharacterSpeed = ConfigFile->GetFloatValue("CharacterSpeed");
	PostThrowTimerLimit = ConfigFile->GetFloatValue("PostThrowTimerLimit");

	DeathAnimationFPS = ConfigFile->GetFloatValue("CharacterDeathAnimatorFPS");
	AliveAnimationsFPS = ConfigFile->GetFloatValue("CharacterAnimatorFPS");
	Animator.FPS = AliveAnimationsFPS;
	PopulateAnimFrames();
}

void Character::Update(float deltaTime, GameInputState inputState)
{
	if (bBeingCrushed)
	{
		// if we're being crushed then the apple crushing us performs our update
		return;
	}

	if (inputState.CrystalBallPress() && CrystalBallState == CrystalBallState::HasBall)
	{
		MyCrystalBall.Release();
		PostThrowTimer = 0.0f;
		bCanCatchBall = false;
		CrystalBallState = CrystalBallState::NoBall;
	}
	if (MyCrystalBall.IsReleased())
	{
		assert(CrystalBallState == CrystalBallState::NoBall);
		MyCrystalBall.Update(deltaTime);
		PostThrowTimer += deltaTime;
		if (PostThrowTimer > PostThrowTimerLimit)
		{
			bCanCatchBall = true;
		}
	}
	if (inputState.AnyDirectionPressed())
	{
		bIsMoving = true;
		if (bHasMoved)
		{
			static ivec2 cellWhenDiggingStarted = { -1,-1 };
			if (CachedTiledWorld->IsBarrierBetween(CurrentTile, DestinationTile))
			{
				PushingState = PushingState::Digging;
				cellWhenDiggingStarted = CurrentTile;
				bPreviousBarrierBetween = true;
			}
			else if(PushingState == PushingState::Digging)
			{
				bPreviousBarrierBetween = false;
				if (CurrentMovementDirection == PreviousDirection && CurrentTile == cellWhenDiggingStarted)
				{
					PushingState = PushingState::Digging;
				}
				else
				{
					PushingState = PushingState::NotPushing;
				}
			}
			else
			{
				PushingState = PushingState::NotPushing;
				bPreviousBarrierBetween = false;
			}
			
			MovementDirection newDir = GetMovementDirection(inputState);
			switch (CurrentMovementDirection)
			{
			case MovementDirection::Down:
				switch (newDir)
				{
				case MovementDirection::Right:
				case MovementDirection::Left:
					NextMovementDirection = newDir;
					if (CurrentTile == DestinationTile) // we're at an edge - allow immediate movement as we will never reach our destination cell
					{
						CurrentMovementDirection = newDir;
						SetNewDestinationCell(newDir);
					}
					break;
				case MovementDirection::Up:
					CurrentMovementDirection = newDir;
					NextMovementDirection = newDir;
					//CurrentTile = DestinationTile;
					SetNewDestinationCell(newDir);
					break;
				}
				break;
			case MovementDirection::Left:
				switch (newDir)
				{
				case MovementDirection::Up:
				case MovementDirection::Down:
					NextMovementDirection = newDir;
					if (CurrentTile == DestinationTile)
					{
						CurrentMovementDirection = newDir;
						SetNewDestinationCell(newDir);
					}
					break;
				case MovementDirection::Right:
					CurrentMovementDirection = newDir;
					NextMovementDirection = newDir;
					//CurrentTile = DestinationTile;
					SetNewDestinationCell(newDir);
					break;
				}
				break;
			case MovementDirection::Right:
				switch (newDir)
				{
				case MovementDirection::Up:
				case MovementDirection::Down:
					NextMovementDirection = newDir;
					if (CurrentTile == DestinationTile)
					{
						CurrentMovementDirection = newDir;
						SetNewDestinationCell(newDir);
					}
					break;
				case MovementDirection::Left:
					CurrentMovementDirection = newDir;
					NextMovementDirection = newDir;
					//CurrentTile = DestinationTile;
					SetNewDestinationCell(newDir);
					break;
				}
				break;
			case MovementDirection::Up:
				switch (newDir)
				{
				case MovementDirection::Down:
					CurrentMovementDirection = newDir;
					NextMovementDirection = newDir;
					//CurrentTile = DestinationTile;
					SetNewDestinationCell(newDir);
					break;
				case MovementDirection::Left:
				case MovementDirection::Right:
					NextMovementDirection = newDir;
					if (CurrentTile == DestinationTile)
					{
						CurrentMovementDirection = newDir;
						SetNewDestinationCell(newDir);
					}
					break;
				}
				break;
			}
			MoveTowardsDestination(deltaTime);
		}
		else
		{
			// first time movement needs to be handled differently as we're not yet fully initialised
			MovementDirection newDir = GetMovementDirection(inputState);
			CurrentMovementDirection = newDir;
			NextMovementDirection = newDir;
			SetNewDestinationCell(newDir);
			MoveTowardsDestination(deltaTime);
			PushingState = CachedTiledWorld->IsBarrierBetween(CurrentTile, DestinationTile) ? PushingState::Digging : PushingState::NotPushing;
			bHasMoved = true;
		}
	}
	else
	{
		bIsMoving = false;
	}

	// find any apples we're colliding with from top or bottom
	assert(AppleManagerRef);
	AppleManager::Apple* foundApple = AppleManagerRef->FindActiveAppleByPredicate([this](const AppleManager::Apple& apple) -> bool
	{
		return CollisionHelpers::AABBCollision(
			CurrentLocation + vec2{ A_SMALL_NUMBER / 2.0f, 0 },
			apple.Position, 
			CachedSpriteDims - vec2{ A_SMALL_NUMBER, 0 }, // take a small number from either side of our dims so we don't collide with apples to the left and right of us
			CachedSpriteDims);
	});

	if (foundApple && foundApple->State == AppleManager::AppleState::Settled)
	{
		if ((CurrentLocation.y > foundApple->Position.y) && (round(CurrentLocation.x) == round(foundApple->Position.x)))
		{
			// we're approaching apple from below therefore it's blocking us - resolve collision
			CurrentLocation.y = foundApple->Position.y + CachedSpriteDims.y;
			bHasMoved = false;
		}
		else if ((CurrentLocation.y < foundApple->Position.y) && (round(CurrentLocation.x) == round(foundApple->Position.x)))
		{
			// we're approaching apple from above therefore it's blocking us - resolve collision
			CurrentLocation.y = foundApple->Position.y - CachedSpriteDims.y;
			bHasMoved = false;
		}
		PushingState = PushingState::Pushing;
	}
	PreviousDirection = CurrentMovementDirection;
	Animator.CurrentAnimation = &RunningAnimFrames[(u32)CrystalBallState][(u32)PushingState][(u32)CurrentMovementDirection];
	Animator.bIsAnimating = bIsMoving;
	Animator.Update(deltaTime / 1000.0f);
}

void Character::CatchBall(bool forceCatch)
{
	assert(CrystalBallState == CrystalBallState::NoBall);
	assert(MyCrystalBall.IsReleased());
	if (bCanCatchBall || forceCatch)
	{
		CrystalBallState = CrystalBallState::HasBall;
		MyCrystalBall.OnCaught();
	}
}

void Character::PopulateAnimFrames()
{
	AnimationAssetManager->MakeAnimationRectFramesFromName("MrDo_RunRight_NoBall", RunningAnimFrames[0][0][1]);
	AnimationAssetManager->MakeAnimationRectFramesFromName("MrDo_RunRight_Ball", RunningAnimFrames[1][0][1]);
	AnimationAssetManager->MakeAnimationRectFramesFromName("MrDo_RunRight_NoBall_Digging", RunningAnimFrames[0][2][1]);
	AnimationAssetManager->MakeAnimationRectFramesFromName("MrDo_RunRight_Ball_Digging", RunningAnimFrames[1][2][1]);
	AnimationAssetManager->MakeAnimationRectFramesFromName("MrDo_RunRight_NoBall_Pushing", RunningAnimFrames[0][1][1]);
	AnimationAssetManager->MakeAnimationRectFramesFromName("MrDo_RunRight_Ball_Pushing", RunningAnimFrames[1][1][1]);

	AnimationAssetManager->MakeAnimationRectFramesFromName("MrDo_RunDown_NoBall", RunningAnimFrames[0][0][2]);
	AnimationAssetManager->MakeAnimationRectFramesFromName("MrDo_RunDown_Ball", RunningAnimFrames[1][0][2]);
	AnimationAssetManager->MakeAnimationRectFramesFromName("MrDo_RunDown_NoBall_Digging", RunningAnimFrames[0][2][2]);
	AnimationAssetManager->MakeAnimationRectFramesFromName("MrDo_RunDown_Ball_Digging", RunningAnimFrames[1][2][2]);
	AnimationAssetManager->MakeAnimationRectFramesFromName("MrDo_RunDown_NoBall_Pushing", RunningAnimFrames[0][1][2]);
	AnimationAssetManager->MakeAnimationRectFramesFromName("MrDo_RunDown_Ball_Pushing", RunningAnimFrames[1][1][2]);

	AnimationAssetManager->MakeAnimationRectFramesFromName("MrDo_RunLeft_NoBall", RunningAnimFrames[0][0][3]);
	AnimationAssetManager->MakeAnimationRectFramesFromName("MrDo_RunLeft_Ball", RunningAnimFrames[1][0][3]);
	AnimationAssetManager->MakeAnimationRectFramesFromName("MrDo_RunLeft_NoBall_Digging", RunningAnimFrames[0][2][3]);
	AnimationAssetManager->MakeAnimationRectFramesFromName("MrDo_RunLeft_Ball_Digging", RunningAnimFrames[1][2][3]);
	AnimationAssetManager->MakeAnimationRectFramesFromName("MrDo_RunLeft_NoBall_Pushing", RunningAnimFrames[0][1][3]);
	AnimationAssetManager->MakeAnimationRectFramesFromName("MrDo_RunLeft_Ball_Pushing", RunningAnimFrames[1][1][3]);

	AnimationAssetManager->MakeAnimationRectFramesFromName("MrDo_RunUp_NoBall", RunningAnimFrames[0][0][0]);
	AnimationAssetManager->MakeAnimationRectFramesFromName("MrDo_RunUp_Ball", RunningAnimFrames[1][0][0]);
	AnimationAssetManager->MakeAnimationRectFramesFromName("MrDo_RunUp_NoBall_Digging", RunningAnimFrames[0][2][0]);
	AnimationAssetManager->MakeAnimationRectFramesFromName("MrDo_RunUp_Ball_Digging", RunningAnimFrames[1][2][0]);
	AnimationAssetManager->MakeAnimationRectFramesFromName("MrDo_RunUp_NoBall_Pushing", RunningAnimFrames[0][1][0]);
	AnimationAssetManager->MakeAnimationRectFramesFromName("MrDo_RunUp_Ball_Pushing", RunningAnimFrames[1][1][0]);

	AnimationAssetManager->MakeAnimationRectFramesFromName("MrDo_Die", DieAnimFrames);

	AnimationAssetManager->MakeSingleSpriteRectFrame("MrDo_Crushed", CrushedFrame);

}

MovementDirection Character::GetMovementDirection(GameInputState inputState)
{
	if (inputState.Down)
	{
		return MovementDirection::Down;
	}
	else if (inputState.Up)
	{
		return MovementDirection::Up;
	}
	else if (inputState.Left)
	{
		return MovementDirection::Left;
	}
	else if (inputState.Right)
	{
		return MovementDirection::Right;
	}
	return MovementDirection::Undefined;
}

void Character::SetAppleManager(AppleManager* appleManager)
{
	AppleManagerRef = appleManager;
}

void Character::SetNewDestinationCell(MovementDirection newDirection)
{
	ivec2 MovementVector;
	switch (newDirection)
	{
	case MovementDirection::Up:
		{
			ivec2 possibleDest = CurrentTile + ivec2{ 0,-1 };
			if (possibleDest.y >= CachedTiledWorld->GetHUDTileRowsTop())
			{
				DestinationTile = possibleDest;
			}
			else
			{
				DestinationTile = CurrentTile;
			}
		}
		break;
	case MovementDirection::Down:
		{
			ivec2 possibleDest = CurrentTile + ivec2{ 0,1 };
			if (possibleDest.y < CachedLevelDims.y - CachedTiledWorld->GetHUDTileRowsBottom())
			{
				DestinationTile = possibleDest;
			}
			else
			{
				DestinationTile = CurrentTile;
			}
		}
		break;
	case MovementDirection::Left:
		{
			ivec2 possibleDest = CurrentTile + ivec2{ -1,0 };
			if (possibleDest.x >= 0)
			{
				DestinationTile = possibleDest;
			}
			else
			{
				DestinationTile = CurrentTile;
			}
		}
		break;
	case MovementDirection::Right:
		{
			ivec2 possibleDest = CurrentTile + ivec2{ 1,0 };
			if (possibleDest.x < CachedLevelDims.x)
			{
				DestinationTile = possibleDest;
			}
			else
			{
				DestinationTile = CurrentTile;
			}
		}
		break;
	}
}

void Character::MoveTowardsDestination(float deltaTime)
{
	// TODO: this function BADLY needs refactoring
	float tileSize = ConfigFile->GetBackgroundConfigData().TileSize;
	float deltaTSeconds = deltaTime / 1000.0f;
	vec2 dVec = MovementHelpers::GetDirectionVector(CurrentMovementDirection);
	if (DestinationTile != CurrentTile)
	{
		float speedMultiplier = 1.0;
		switch (PushingState)
		{
		case PushingState::Pushing:
			speedMultiplier = PushSpeedMultiplier;
			break;
		case PushingState::Digging:
			speedMultiplier = DigSpeedMultiplier;
			break;
		}
		CurrentLocation += (dVec * CharacterSpeed * deltaTime * speedMultiplier);
	}
	switch (CurrentMovementDirection)
	{
	case MovementDirection::Up:
		if (CurrentLocation.y < CurrentTile.y * tileSize - tileSize / 2.0f)
		{
			// over half way to next tile - break walls
			CachedTiledWorld->ConnectAdjacentCells(CurrentTile, DestinationTile);
			if (CachedTiledWorld->IsCherryAtTile(DestinationTile))
			{
				CachedTiledWorld->RemoveCherryAtTile(DestinationTile);
				// send cherry collected message here
			}
		}
		if (CurrentLocation.y < CurrentTile.y * tileSize - tileSize)
		{
			// all the way - set next tile dest
			CurrentLocation.y = CurrentTile.y * tileSize - tileSize;
			CurrentMovementDirection = NextMovementDirection;
			PreviousTile = CurrentTile;
			CurrentTile = DestinationTile;
			SetNewDestinationCell(CurrentMovementDirection);
		}
		break;
	case MovementDirection::Down:
		if (CurrentLocation.y > CurrentTile.y * tileSize + tileSize / 2.0f)
		{
			// over half way to next tile - break walls
			CachedTiledWorld->ConnectAdjacentCells(CurrentTile, DestinationTile);
			if (CachedTiledWorld->IsCherryAtTile(DestinationTile))
			{
				CachedTiledWorld->RemoveCherryAtTile(DestinationTile);
				// send cherry collected message here
			}
		}
		if (CurrentLocation.y > CurrentTile.y * tileSize + tileSize)
		{
			// all the way - set next tile dest
			CurrentLocation.y = CurrentTile.y * tileSize + tileSize;
			CurrentMovementDirection = NextMovementDirection;
			PreviousTile = CurrentTile;
			CurrentTile = DestinationTile;
			SetNewDestinationCell(CurrentMovementDirection);
		}
		break;
	case MovementDirection::Left:
		if (CurrentLocation.x < CurrentTile.x * tileSize - tileSize / 2.0f)
		{
			// over half way to next tile - break walls
			CachedTiledWorld->ConnectAdjacentCells(CurrentTile, DestinationTile);
			if (CachedTiledWorld->IsCherryAtTile(DestinationTile))
			{
				CachedTiledWorld->RemoveCherryAtTile(DestinationTile);
				// send cherry collected message here
			}
		}
		if (CurrentLocation.x < CurrentTile.x * tileSize - tileSize)
		{
			// all the way - set next tile dest
			CurrentLocation.x = CurrentTile.x * tileSize - tileSize;
			CurrentMovementDirection = NextMovementDirection;
			PreviousTile = CurrentTile;
			CurrentTile = DestinationTile;
			SetNewDestinationCell(CurrentMovementDirection);
		}
		break;
	case MovementDirection::Right:
		if (CurrentLocation.x > CurrentTile.x * tileSize + tileSize / 2.0f)
		{
			// over half way to next tile - break walls
			CachedTiledWorld->ConnectAdjacentCells(CurrentTile, DestinationTile);
			if (CachedTiledWorld->IsCherryAtTile(DestinationTile))
			{
				CachedTiledWorld->RemoveCherryAtTile(DestinationTile);
				// send cherry collected message here
			}
		}
		if (CurrentLocation.x > CurrentTile.x * tileSize + tileSize)
		{
			// all the way - set next tile dest
			CurrentLocation.x = CurrentTile.x * tileSize + tileSize;
			CurrentMovementDirection = NextMovementDirection;
			PreviousTile = CurrentTile;
			CurrentTile = DestinationTile;
			SetNewDestinationCell(CurrentMovementDirection);
		}
		break;
	}
}

void Character::Kill(CharacterDeathReason deathReason)
{
	bBeingCrushed = false;
	// set animator state for death animation
	Animator.bIsAnimating = true;
	Animator.CurrentAnimation = &DieAnimFrames;
	Animator.OnAnimFrame = 0;
	Animator.FPS = DeathAnimationFPS;
	Animator.bLooping = false;
	Animator.bFinished = false;

	GameFramework::SendFrameworkMessage(CharacterDied{ deathReason });
}

void Character::Crush()
{
	assert(!bBeingCrushed);
	bBeingCrushed = true;
}

void Character::OnNewLevelStarted(LevelLoadData level)
{
	OnResetAfterDeath(level);

	CachedLevelDims.x = CachedTiledWorld->GetActiveLevelWidth(); // todo: sort these out - sort out size of levels in general
	CachedLevelDims.y = CachedTiledWorld->GetActiveLevelHeight();

	u8& spawnedAtTile = CachedTiledWorld->GetCellAtIndex(CurrentTile);

	spawnedAtTile &= ~(1 << (u32)TileWallDirectionBit::Center); // knock out center wall of tile spawned in

	Animator.CurrentAnimation = &RunningAnimFrames[0][0][0];
	if (bCanCatchBall)
	{
		CatchBall();
	}
}

void Character::OnResetAfterDeath(LevelLoadData levelLoadData)
{
	const std::vector<LevelConfigData>& data = levelLoadData.Source == LevelSource::ArcadeLevels ? ConfigFile->GetLevelsConfigData() : ConfigFile->GetMapMakerLevelsConfigData();

	const LevelConfigData& level = data[levelLoadData.LevelIndex];
	const BackgroundTileConfigData& bgData = ConfigFile->GetBackgroundConfigData();

	CurrentTile = ivec2{ (int)level.PlayerSpawnLocation.x, (int)level.PlayerSpawnLocation.y };
	CurrentLocation = vec2{ (float)level.PlayerSpawnLocation.x * bgData.TileSize, (float)level.PlayerSpawnLocation.y * bgData.TileSize };
	CurrentMovementDirection = (MovementDirection)level.PlayerSpawnFacing;

	// reset flags
	bBeingCrushed = false;
	bIsMoving = false;
	bHasMoved = false;
	bCanCatchBall = false;
	PushingState = PushingState::NotPushing;

	// set animator state for gameplay
	Animator.OnAnimFrame = 0;
	Animator.FPS = AliveAnimationsFPS;
	Animator.bLooping = true;
	Animator.bFinished = false;

	// make sure ball is caught
	if (MyCrystalBall.IsReleased())
	{
		CatchBall();
	}
	MyCrystalBall.ResetStateOnDeath();
}

void Character::Draw(SDL_Surface* windowSurface, float scale) const
{
	if (MyCrystalBall.IsReleased())
	{
		assert(CrystalBallState == CrystalBallState::NoBall);
		MyCrystalBall.Draw(windowSurface, scale);
	}
	SDL_Surface* surface = AnimationAssetManager->GetAnimationsSpriteSheetSurface();
	SDL_Rect dst;
	float tileSize = ConfigFile->GetBackgroundConfigData().TileSize;
	dst.w = tileSize * scale;
	dst.h = tileSize * scale;
	dst.x = CurrentLocation.x * scale;
	dst.y = CurrentLocation.y * scale;

	const SDL_Rect* rect;
	if (bBeingCrushed)
	{
		rect = &CrushedFrame;
	}
	else
	{
		rect = &Animator.GetCurrentFrame();
	}
	
	SDL_BlitSurfaceScaled(surface, rect, windowSurface, &dst);
}

void Character::UpdatePlayingDeathAnimation(float deltaTime)
{
	Animator.Update(deltaTime / 1000.0f);
}
