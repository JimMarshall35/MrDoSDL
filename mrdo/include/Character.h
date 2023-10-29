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
#include "EventListener.h"
#include "GameFrameworkMessages.h"
#include "LevelLoadData.h"

class IAnimationAssetManager;
class TiledWorld;
class AppleManager;
class IConfigFile;

template<typename T>
class Event;

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
	Character(
		const std::shared_ptr<IAnimationAssetManager>& assetManager,
		const std::shared_ptr<IConfigFile>& configFile, 
		const std::shared_ptr<TiledWorld>& tiledWorld,
		Event<LevelLoadData>& onLevelLoaded,
		Event<LevelLoadData>& onResetAfterDeath);
	void Update(float deltaTime, GameInputState inputState);
	void UpdatePlayingDeathAnimation(float deltaTime);
	void Draw(SDL_Surface* windowSurface, float scale) const;
	vec2 GetPosition() const { return CurrentLocation; }
	void SetPosition(const vec2& pos) { CurrentLocation = pos; }

	MovementDirection GetCurrentMovementDirection() const { return CurrentMovementDirection; }
	void CatchBall();
	const ivec2& GetTile() const { return CurrentTile; }
	void SetAppleManager(AppleManager* appleManager);     // can't be passed into ctor with other dependencies as both this and the apple manager are members of GameLayer and instantiated at same time so would lead to chicken and egg syndrom
	void Kill(CharacterDeathReason deathReason);
	void Crush();
	bool IsAnimationFinished() const { return Animator.bFinished; }
private:
	void PopulateAnimFrames();
	MovementDirection GetMovementDirection(GameInputState inputState);
	void SetNewDestinationCell(MovementDirection newDirection);
	void MoveTowardsDestination(float deltaTime);
	void OnNewLevelStarted(LevelLoadData level);
	void OnResetAfterDeath(LevelLoadData level);
private:
	CrystalBall MyCrystalBall;
	Animator Animator;
	// map of frames, index in by crystal ball state, is pushing or digging and finally direction moving
	std::vector<SDL_Rect> RunningAnimFrames[2][3][4];
	SDL_Rect CrushedFrame;
	std::vector<SDL_Rect> DieAnimFrames;
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
	u8 bBeingCrushed : 1;

	LISTENER(Character, OnNewLevelStarted, LevelLoadData);
	LISTENER(Character, OnResetAfterDeath, LevelLoadData);
	vec2 CachedSpriteDims;

	AppleManager* AppleManagerRef = nullptr;

	float DeathAnimationFPS;
	float AliveAnimationsFPS;
};