#include "Character.h"
#include "IAnimationAssetManager.h"
#include "IConfigFile.h"
#include "TiledWorld.h"
#include <functional>
#include <cassert>

Character::Character(const std::shared_ptr<IAnimationAssetManager>& assetManager, const std::shared_ptr<IConfigFile>& configFile, const std::shared_ptr<TiledWorld>& tiledWorld)
	:AnimationAssetManager(assetManager),
	ConfigFile(configFile),
	CachedTiledWorld(tiledWorld)
{
	const std::vector<LevelConfigData>& data = ConfigFile->GetLevelsConfigData();
	const LevelConfigData& level = data[CachedTiledWorld->GetLevelLoaded()];
	const BackgroundTileConfigData& bgData = ConfigFile->GetBackgroundConfigData();

	CurrentTile = ivec2{ (int)level.PlayerSpawnLocation.x, (int)level.PlayerSpawnLocation.x };
	CurrentLocation = vec2{ (float)level.PlayerSpawnLocation.x * bgData.TileSize, (float)level.PlayerSpawnLocation.y * bgData.TileSize };
	CurrentMovementDirection = (MovementDirection)level.PlayerSpawnFacing;
	bIsMoving = false;
	bHasMoved = false;
	CachedLevelDims.x = CachedTiledWorld->GetActiveLevelWidth();
	CachedLevelDims.y = CachedTiledWorld->GetActiveLevelHeight();

	u8& spawnedAtTile = CachedTiledWorld->GetCellAtIndex(CurrentTile);

	spawnedAtTile &= ~(1 << (u32)TileWallDirectionBit::Center); // knock out center wall of tile spawned in

	CharacterSpeed = ConfigFile->GetFloatValue("CharacterSpeed");
	PopulateAnimFrames();
}

void Character::Update(float deltaTime, GameInputState inputState)
{
	if (inputState.AnyDirectionPressed())
	{
		bIsMoving = true;
		if (bHasMoved)
		{
			PushingState = CachedTiledWorld->IsBarrierBetween(CurrentTile, DestinationTile) ? PushingState::Digging : PushingState::NotPushing;
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
					CurrentTile = DestinationTile;
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
					CurrentTile = DestinationTile;
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
					CurrentTile = DestinationTile;
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
					CurrentTile = DestinationTile;
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

void Character::SetNewDestinationCell(MovementDirection newDirection)
{
	ivec2 MovementVector;
	switch (newDirection)
	{
	case MovementDirection::Up:
		{
			ivec2 possibleDest = CurrentTile + ivec2{ 0,-1 };
			if (possibleDest.y >= 0)
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
			if (possibleDest.y < CachedLevelDims.y)
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
	
	float tileSize = ConfigFile->GetBackgroundConfigData().TileSize;
	float deltaTSeconds = deltaTime / 1000.0f;
	vec2 dVec = GetDirectionVector(CurrentMovementDirection);
	if (DestinationTile != CurrentTile)
	{
		CurrentLocation += (dVec * CharacterSpeed * deltaTime);
	}
	switch (CurrentMovementDirection)
	{
	case MovementDirection::Up:
		if (CurrentLocation.y < CurrentTile.y * tileSize - tileSize / 2.0f)
		{
			// over half way to next tile - break walls
			CachedTiledWorld->ConnectAdjacentCells(CurrentTile, DestinationTile);
		}
		if (CurrentLocation.y < CurrentTile.y * tileSize - tileSize)
		{
			// all the way - set next tile dest
			CurrentLocation.y = CurrentTile.y * tileSize - tileSize;
			CurrentMovementDirection = NextMovementDirection;
			CurrentTile = DestinationTile;
			SetNewDestinationCell(CurrentMovementDirection);
		}
		break;
	case MovementDirection::Down:
		if (CurrentLocation.y > CurrentTile.y * tileSize + tileSize / 2.0f)
		{
			// over half way to next tile - break walls
			CachedTiledWorld->ConnectAdjacentCells(CurrentTile, DestinationTile);
		}
		if (CurrentLocation.y > CurrentTile.y * tileSize + tileSize)
		{
			// all the way - set next tile dest
			CurrentLocation.y = CurrentTile.y * tileSize + tileSize;
			CurrentMovementDirection = NextMovementDirection;
			CurrentTile = DestinationTile;
			SetNewDestinationCell(CurrentMovementDirection);
		}
		break;
	case MovementDirection::Left:
		if (CurrentLocation.x < CurrentTile.x * tileSize - tileSize / 2.0f)
		{
			// over half way to next tile - break walls
			CachedTiledWorld->ConnectAdjacentCells(CurrentTile, DestinationTile);
		}
		if (CurrentLocation.x < CurrentTile.x * tileSize - tileSize)
		{
			// all the way - set next tile dest
			CurrentLocation.x = CurrentTile.x * tileSize - tileSize;
			CurrentMovementDirection = NextMovementDirection;
			CurrentTile = DestinationTile;
			SetNewDestinationCell(CurrentMovementDirection);
		}
		break;
	case MovementDirection::Right:
		if (CurrentLocation.x > CurrentTile.x * tileSize + tileSize / 2.0f)
		{
			// over half way to next tile - break walls
			CachedTiledWorld->ConnectAdjacentCells(CurrentTile, DestinationTile);
		}
		if (CurrentLocation.x > CurrentTile.x * tileSize + tileSize)
		{
			// all the way - set next tile dest
			CurrentLocation.x = CurrentTile.x * tileSize + tileSize;
			CurrentMovementDirection = NextMovementDirection;
			CurrentTile = DestinationTile;
			SetNewDestinationCell(CurrentMovementDirection);
		}
		break;
	}
}

vec2 Character::GetDirectionVector(MovementDirection direction)
{
	static const vec2 LUT[4] = { {0,-1},{1,0},{0,1},{-1,0} };
	u32 i = (u32)direction;
	assert(i < 4);
	return LUT[i];
}

void Character::Draw(SDL_Surface* windowSurface, float scale) const
{
	SDL_Surface* surface = AnimationAssetManager->GetAnimationsSpriteSheetSurface();
	SDL_Rect dst;
	float tileSize = ConfigFile->GetBackgroundConfigData().TileSize;
	dst.w = tileSize * scale;
	dst.h = tileSize * scale;
	dst.x = CurrentLocation.x * scale;
	dst.y = CurrentLocation.y * scale;
	const std::vector<SDL_Rect>& animation = RunningAnimFrames[(u32)CrystalBallState][(u32)PushingState][(u32)CurrentMovementDirection];
	const SDL_Rect& rect = animation[Animator.OnAnimFrame];
	SDL_BlitSurfaceScaled(surface, &rect, windowSurface, &dst);
}
