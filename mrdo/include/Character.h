#pragma once
#include <memory>
#include <vector>
#include "SDL.h"
#include "CommonTypedefs.h"
#include "InputManager.h"
#include "VectorTypes.h"
#include "Animator.h"
#include "MovementTypes.h"
#include "CrystalBall.h"

class IAnimationAssetManager;
class IConfigFile;
class TiledWorld;

enum class CrystalBallState
{
	NoBall = 0,
	HasBall = 1,
	MAX
};

enum class PushingState
{
	NotPushing,
	Pushing,
	Digging
};

struct CharacterInitOptions
{
	uvec2 SpawnLocation;
	MovementDirection SpawnFacing;
};



class Character
{
public:
	Character(const std::shared_ptr<IAnimationAssetManager>& assetManager, const std::shared_ptr<IConfigFile>& configFile, const std::shared_ptr<TiledWorld>& tiledWorld);
	void Update(float deltaTime, GameInputState inputState);
	void Draw(SDL_Surface* windowSurface, float scale) const;
	vec2 GetPosition() const { return CurrentLocation; }
	MovementDirection GetCurrentMovementDirection() const { return CurrentMovementDirection; }
	void CatchBall();
private:
	void PopulateAnimFrames();
	MovementDirection GetMovementDirection(GameInputState inputState);
	void SetNewDestinationCell(MovementDirection newDirection);
	void MoveTowardsDestination(float deltaTime);
private:
	CrystalBall MyCrystalBall;
	Animator Animator;
	// map of frames, index in by crystal ball state, is pushing or digging and finally direction moving
	std::vector<SDL_Rect> RunningAnimFrames[2][3][4];
	std::shared_ptr<IAnimationAssetManager> AnimationAssetManager;
	std::shared_ptr<IConfigFile> ConfigFile;
	std::shared_ptr<TiledWorld> CachedTiledWorld;
	uvec2 CachedLevelDims;
	vec2 CurrentLocation;
	ivec2 CurrentTile;
	ivec2 DestinationTile;
	MovementDirection CurrentMovementDirection;
	MovementDirection NextMovementDirection;
	float CharacterSpeed;
	CrystalBallState CrystalBallState = CrystalBallState::HasBall;
	PushingState PushingState = PushingState::NotPushing;
	float PostThrowTimerLimit;
	float PostThrowTimer;

	u8 bIsMoving : 1;
	u8 bHasMoved : 1;
	u8 bCanCatchBall : 1;
};