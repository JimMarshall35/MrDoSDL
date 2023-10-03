#pragma once
#include "VectorTypes.h"
#include "SDL.h"
#include "CommonTypedefs.h"

class Character;
class IAnimationAssetManager;
class IConfigFile;
class TiledWorld;

class CrystalBall
{
public:
	CrystalBall(Character* owner, IAnimationAssetManager* anim, IConfigFile* configFile, TiledWorld* tiledWorld);
	void Draw(SDL_Surface* windowSurface, float scale) const;
	void Release();
	void Update(float deltaT);
	bool IsReleased() const { return bIsReleased; };
	void SetIsReleased(bool val) { bIsReleased = val; }
private:
	bool DoesBallsPathIntersectLineSegment(
		const vec2& oldBallCenter, const vec2& ballcenter,
		const vec2& segmentPtA, const vec2& segmentPtB,
		const vec2& normalToSetIfItDoes, const ivec2& collidedCellToSetIfCollision,
		bool& hasCollidedOut, vec2& collisionNormalOut, ivec2& collidedCellOut) const;
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
};