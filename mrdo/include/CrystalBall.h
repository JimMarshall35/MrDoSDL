#pragma once
#include "VectorTypes.h"
#include "SDL.h"
#include "CommonTypedefs.h"

class Character;
class IAnimationAssetManager;
class IConfigFile;
class TiledWorld;
class EnemyManager;

class CrystalBall
{
public:
	CrystalBall(Character* owner, IAnimationAssetManager* anim, IConfigFile* configFile, TiledWorld* tiledWorld, EnemyManager* enemyManager);
	void Draw(SDL_Surface* windowSurface, float scale) const;
	void Release();
	void Update(float deltaT);
	bool IsReleased() const { return bIsReleased; };
	void OnCaught();
private:
	void UpdateInternal(float deltaT);
private:
	Character* Owner;
	vec2 Position;
	vec2 DirectionVector;
	IAnimationAssetManager* AnimationAssetManager;
	IConfigFile* ConfigFile;
	bool bIsReleased;
	SDL_Rect BounceUpSprite;
	SDL_Rect BounceDownSprite;
	float CrystalBallSpeed;
	float CachedTileSize;
	TiledWorld* CachedTiledWorld;
	u32 CrystalBallRadius;
	u32 NumUpdatesPerFrame;
	EnemyManager* CachedEnemyManager;
};