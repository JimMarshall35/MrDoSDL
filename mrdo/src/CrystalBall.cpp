#include "CrystalBall.h"
#include "Character.h"
#include "SDL.h"
#include "MovementHelpers.h"
#include "IConfigFile.h"
#include "IAnimationAssetManager.h"
#include "TiledWorld.h"
#include "CollisionHelpers.h"
#include <cassert>
#include <math.h>
#include <algorithm>

CrystalBall::CrystalBall(Character* owner, IAnimationAssetManager* anim, IConfigFile* configFile, TiledWorld* tiledWorld)
	:Owner(owner),
	Position({0,0}),
	AnimationAssetManager(anim),
	ConfigFile(configFile),
	bIsReleased(false),
	CachedTileSize(ConfigFile->GetBackgroundConfigData().TileSize),
	CachedTiledWorld(tiledWorld)
{
	AnimationAssetManager->MakeSingleSpriteRectFrame("CrystalBall_BounceUp", BounceUpSprite);
	AnimationAssetManager->MakeSingleSpriteRectFrame("CrystalBall_BounceDown", BounceDownSprite);
	CrystalBallSpeed = ConfigFile->GetFloatValue("CrystalBallSpeed");
	CrystalBallRadius = ConfigFile->GetUIntValue("CrystalBallRadiusPixels");
	NumUpdatesPerFrame = ConfigFile->GetUIntValue("NumCrystalBallUpdates");
}

void CrystalBall::Draw(SDL_Surface* windowSurface, float scale) const
{
	if (bIsReleased)
	{
		const SDL_Rect* rectToDraw = nullptr;
		if (DirectionVector.y < 0)
		{
			if (DirectionVector.x < 0)
			{
				rectToDraw = &BounceDownSprite;
			}
			else
			{
				rectToDraw = &BounceUpSprite;
			}
		}
		else
		{
			if (DirectionVector.x < 0)
			{
				rectToDraw = &BounceUpSprite;
			}
			else
			{
				rectToDraw = &BounceDownSprite;
			}
		}
		SDL_Surface* surface = AnimationAssetManager->GetAnimationsSpriteSheetSurface();
		SDL_Rect dst;
		float tileSize = ConfigFile->GetBackgroundConfigData().TileSize;
		dst.w = tileSize * scale;
		dst.h = tileSize * scale;
		dst.x = Position.x * scale;
		dst.y = Position.y * scale;
		SDL_BlitSurfaceScaled(surface, rectToDraw, windowSurface, &dst);
	}
}

void CrystalBall::Release()
{
	static const vec2 ReleaseNormalLUT[4] =
	{
		MovementHelpers::Right,
		MovementHelpers::Down,
		MovementHelpers::Right,
		MovementHelpers::Down,
	};
	MovementDirection dir = Owner->GetCurrentMovementDirection();
	vec2 characterFacing = MovementHelpers::GetDirectionVector(dir);
	Position = Owner->GetPosition();
	vec2 releaseNormal = ReleaseNormalLUT[(u32)dir];
	DirectionVector = (characterFacing + vec2{releaseNormal.x * 1.0f, releaseNormal.y*1.0f}).Normalized();
	bIsReleased = true;
}

void CrystalBall::Update(float deltaT)
{
	
	for (int i = 0; i < NumUpdatesPerFrame; i++)
	{
		UpdateInternal(deltaT / NumUpdatesPerFrame);
	}
}

void CrystalBall::OnCaught()
{
	assert(bIsReleased);
	bIsReleased = false;
}

void CrystalBall::UpdateInternal(float deltaT)
{
	if (!bIsReleased)
	{
		return;
	}

	vec2 oldballCenter = Position + vec2{ CachedTileSize / 2.0f, CachedTileSize / 2.0f };
	vec2 oldBallCenter1By1TileIndex = vec2{ oldballCenter.x / CachedTileSize, oldballCenter.y / CachedTileSize };
	uvec2 oldBallCenterTileCoords = uvec2{ (u32)oldBallCenter1By1TileIndex.x, (u32)oldBallCenter1By1TileIndex.y };
	vec2 oldPos = Position;
	Position += (DirectionVector * CrystalBallSpeed * deltaT);

	vec2 ballCenter = Position + vec2{ CachedTileSize / 2.0f, CachedTileSize / 2.0f };
	vec2 ballCenter1By1TileIndex = vec2{ ballCenter.x / CachedTileSize, ballCenter.y / CachedTileSize };
	uvec2 ballCenterTileCoords = uvec2{ (u32)ballCenter1By1TileIndex.x, (u32)ballCenter1By1TileIndex.y };

	if (ballCenterTileCoords != oldBallCenterTileCoords)
	{
		u8 oldBallCenterTile = CachedTiledWorld->GetCellAtIndex(ivec2{ (i32)oldBallCenterTileCoords.x, (i32)oldBallCenter1By1TileIndex.y });
		u8 ballCenterTile = CachedTiledWorld->GetCellAtIndex(ivec2{ (i32)ballCenterTileCoords.x, (i32)ballCenterTileCoords.y });

		int dx = ballCenterTileCoords.x - oldBallCenterTileCoords.x;
		int dy = ballCenterTileCoords.y - oldBallCenterTileCoords.y;
		assert((dx >= -1) && (dx <= 1));
		assert((dy >= -1) && (dy <= 1));

		bool hasCollided = false;
		vec2 collisionNormal;
		ivec2 collidedCell;
		switch (dx)
		{
		case -1:
			switch (dy)
			{
			case -1:
			{
				ivec2 cellToLeft = ivec2{ (i32)oldBallCenterTileCoords.x, (i32)oldBallCenterTileCoords.y } + ivec2{ -1,0 };
				ivec2 cellAbove = ivec2{ (i32)oldBallCenterTileCoords.x, (i32)oldBallCenterTileCoords.y } + ivec2{ 0,-1 };

				u8 tileToLeft = CachedTiledWorld->GetCellAtIndex(cellToLeft);
				u8 tileAbove = CachedTiledWorld->GetCellAtIndex(cellAbove);
				if ((tileAbove & (1 << (u32)TileWallDirectionBit::Down)) || (oldBallCenterTile & (1 << (u32)TileWallDirectionBit::Up)))
				{
					vec2 oldTileToptWallA = vec2{
						(float)oldBallCenterTileCoords.x * CachedTileSize,
						(float)oldBallCenterTileCoords.y * CachedTileSize
					};
					vec2 oldTileToptWallB = vec2{
						(float)oldBallCenterTileCoords.x * CachedTileSize + CachedTileSize,
						(float)oldBallCenterTileCoords.y * CachedTileSize
					};
					if (CollisionHelpers::isintersect(oldTileToptWallA, oldTileToptWallB, oldballCenter, ballCenter))
					{
						hasCollided = true;
						collidedCell = cellAbove;
						collisionNormal = MovementHelpers::Down;
						break;
					}
				}
				if ((oldBallCenterTile & (1 << (u32)TileWallDirectionBit::Left)) || (tileToLeft & (1 << (u32)TileWallDirectionBit::Right)))
				{
					vec2 oldTileLeftWallA = vec2{
						(float)oldBallCenterTileCoords.x * CachedTileSize,
						(float)oldBallCenterTileCoords.y * CachedTileSize
					};
					vec2 oldTileLeftWallB = vec2{
						(float)oldBallCenterTileCoords.x * CachedTileSize,
						(float)oldBallCenterTileCoords.y * CachedTileSize + CachedTileSize
					};

					if (CollisionHelpers::isintersect(oldTileLeftWallA, oldTileLeftWallB, oldballCenter, ballCenter))
					{
						hasCollided = true;
						collidedCell = cellToLeft;
						collisionNormal = MovementHelpers::Right;
						break;
					}
				}
				if ((ballCenterTile & (1 << (u32)TileWallDirectionBit::Down)) || (tileToLeft & (1 << (u32)TileWallDirectionBit::Up)))
				{
					vec2 newTileBottomWallA = vec2{
						(float)ballCenterTileCoords.x * CachedTileSize,
						(float)ballCenterTileCoords.y * CachedTileSize + CachedTileSize
					};
					vec2 newTileBottomWallB = vec2{
						(float)ballCenterTileCoords.x * CachedTileSize + CachedTileSize,
						(float)ballCenterTileCoords.y * CachedTileSize + CachedTileSize
					};
					if (CollisionHelpers::isintersect(newTileBottomWallA, newTileBottomWallB, oldballCenter, ballCenter))
					{
						hasCollided = true;
						collidedCell = ivec2{ (i32)ballCenterTileCoords.x,  (i32)ballCenterTileCoords.y };
						collisionNormal = MovementHelpers::Down;
						break;
					}
				}
				if ((ballCenterTile & (1 << (u32)TileWallDirectionBit::Right)) || (tileAbove & (1 << (u32)TileWallDirectionBit::Left)))
				{
					vec2 newTileBRightWallA = vec2{
						(float)ballCenterTileCoords.x * CachedTileSize + CachedTileSize,
						(float)ballCenterTileCoords.y * CachedTileSize
					};
					vec2 newTileRightWallB = vec2{
						(float)ballCenterTileCoords.x * CachedTileSize + CachedTileSize,
						(float)ballCenterTileCoords.y * CachedTileSize + CachedTileSize
					};
					if (CollisionHelpers::isintersect(newTileBRightWallA, newTileRightWallB, oldballCenter, ballCenter))
					{
						hasCollided = true;
						collidedCell = ivec2{ (i32)ballCenterTileCoords.x,  (i32)ballCenterTileCoords.y };
						collisionNormal = MovementHelpers::Right;
						break;
					}
				}
			}

			break;
			case 0:
				if ((oldBallCenterTile & (1 << (u32)TileWallDirectionBit::Left)) || (ballCenterTile & (1 << (u32)TileWallDirectionBit::Right)))
				{
					// collision
					hasCollided = true;
					collidedCell = ivec2{ (i32)ballCenterTileCoords.x, (i32)ballCenterTileCoords.y };
					collisionNormal = MovementHelpers::Right;
				}
				break;
			case 1:
			{

				ivec2 cellToLeft = ivec2{ (i32)oldBallCenterTileCoords.x, (i32)oldBallCenterTileCoords.y } + ivec2{ -1,0 };
				ivec2 cellBelow = ivec2{ (i32)oldBallCenterTileCoords.x, (i32)oldBallCenterTileCoords.y } + ivec2{ 0, 1 };

				u8 tileToLeft = CachedTiledWorld->GetCellAtIndex(cellToLeft);
				u8 tileBelow = CachedTiledWorld->GetCellAtIndex(cellBelow);
				if ((tileBelow & (1 << (u32)TileWallDirectionBit::Up)) || (oldBallCenterTile & (1 << (u32)TileWallDirectionBit::Down)))
				{
					vec2 oldTilBottomtWallA = vec2{
						(float)oldBallCenterTileCoords.x * CachedTileSize,
						(float)oldBallCenterTileCoords.y * CachedTileSize + CachedTileSize
					};
					vec2 oldTilBottomtWallB = vec2{
						(float)oldBallCenterTileCoords.x * CachedTileSize + CachedTileSize,
						(float)oldBallCenterTileCoords.y * CachedTileSize + CachedTileSize
					};
					if (CollisionHelpers::isintersect(oldTilBottomtWallA, oldTilBottomtWallB, oldballCenter, ballCenter))
					{
						hasCollided = true;
						collidedCell = cellBelow;
						collisionNormal = MovementHelpers::Up;
						break;
					}
				}
				if ((oldBallCenterTile & (1 << (u32)TileWallDirectionBit::Left)) || (tileToLeft & (1 << (u32)TileWallDirectionBit::Right)))
				{
					vec2 oldTileLeftWallA = vec2{
						(float)oldBallCenterTileCoords.x * CachedTileSize,
						(float)oldBallCenterTileCoords.y * CachedTileSize
					};
					vec2 oldTileLeftWallB = vec2{
						(float)oldBallCenterTileCoords.x * CachedTileSize,
						(float)oldBallCenterTileCoords.y * CachedTileSize + CachedTileSize
					};

					if (CollisionHelpers::isintersect(oldTileLeftWallA, oldTileLeftWallB, oldballCenter, ballCenter))
					{
						hasCollided = true;
						collidedCell = cellToLeft;
						collisionNormal = MovementHelpers::Right;
						break;
					}
				}
				if ((ballCenterTile & (1 << (u32)TileWallDirectionBit::Up)) || (tileToLeft & (1 << (u32)TileWallDirectionBit::Down)))
				{
					vec2 newTileTopWallA = vec2{
						(float)ballCenterTileCoords.x * CachedTileSize,
						(float)ballCenterTileCoords.y * CachedTileSize
					};
					vec2 newTileTopWallB = vec2{
						(float)ballCenterTileCoords.x * CachedTileSize + CachedTileSize,
						(float)ballCenterTileCoords.y * CachedTileSize
					};
					if (CollisionHelpers::isintersect(newTileTopWallA, newTileTopWallB, oldballCenter, ballCenter))
					{
						hasCollided = true;
						collidedCell = ivec2{ (i32)ballCenterTileCoords.x,  (i32)ballCenterTileCoords.y };
						collisionNormal = MovementHelpers::Up;
						break;
					}
				}
				if ((ballCenterTile & (1 << (u32)TileWallDirectionBit::Right)) || (tileBelow & (1 << (u32)TileWallDirectionBit::Left)))
				{
					vec2 newTileRightWallA = vec2{
						(float)ballCenterTileCoords.x * CachedTileSize + CachedTileSize,
						(float)ballCenterTileCoords.y * CachedTileSize
					};
					vec2 newTileRightWallB = vec2{
						(float)ballCenterTileCoords.x * CachedTileSize + CachedTileSize,
						(float)ballCenterTileCoords.y * CachedTileSize + CachedTileSize
					};
					if (CollisionHelpers::isintersect(newTileRightWallA, newTileRightWallB, oldballCenter, ballCenter))
					{
						hasCollided = true;
						collidedCell = ivec2{ (i32)ballCenterTileCoords.x,  (i32)ballCenterTileCoords.y };
						collisionNormal = MovementHelpers::Right;
						break;
					}
				}
			}
			break;
			}
			break;
		case 0:
			switch (dy)
			{
			case -1:
				if ((oldBallCenterTile & (1 << (u32)TileWallDirectionBit::Up)) || (ballCenterTile & (1 << (u32)TileWallDirectionBit::Down)))
				{
					// collision
					hasCollided = true;
					collidedCell = ivec2{ (i32)ballCenterTileCoords.x, (i32)ballCenterTileCoords.y };
					collisionNormal = MovementHelpers::Down;
				}
				break;
			case 0:
				break;
			case 1:
				if ((oldBallCenterTile & (1 << (u32)TileWallDirectionBit::Down)) || (ballCenterTile & (1 << (u32)TileWallDirectionBit::Up)))
				{
					// collision
					hasCollided = true;
					collidedCell = ivec2{ (i32)ballCenterTileCoords.x, (i32)ballCenterTileCoords.y };
					collisionNormal = MovementHelpers::Up;
				}
				break;
			}
			break;
		case 1:
			switch (dy)
			{
			case -1:
			{
				ivec2 cellToRight = ivec2{ (i32)oldBallCenterTileCoords.x, (i32)oldBallCenterTileCoords.y } + ivec2{ 1,0 };
				ivec2 cellAbove = ivec2{ (i32)oldBallCenterTileCoords.x, (i32)oldBallCenterTileCoords.y } + ivec2{ 0,-1 };

				u8 tileToRight = CachedTiledWorld->GetCellAtIndex(cellToRight);
				u8 tileAbove = CachedTiledWorld->GetCellAtIndex(cellAbove);
				if ((tileAbove & (1 << (u32)TileWallDirectionBit::Down)) || (oldBallCenterTile & (1 << (u32)TileWallDirectionBit::Up)))
				{
					vec2 oldTileToptWallA = vec2{
						(float)oldBallCenterTileCoords.x * CachedTileSize,
						(float)oldBallCenterTileCoords.y * CachedTileSize
					};
					vec2 oldTileToptWallB = vec2{
						(float)oldBallCenterTileCoords.x * CachedTileSize + CachedTileSize,
						(float)oldBallCenterTileCoords.y * CachedTileSize
					};
					if (CollisionHelpers::isintersect(oldTileToptWallA, oldTileToptWallB, oldballCenter, ballCenter))
					{
						hasCollided = true;
						collidedCell = cellToRight;
						collisionNormal = MovementHelpers::Down;
						break;
					}
				}
				if ((oldBallCenterTile & (1 << (u32)TileWallDirectionBit::Right)) || (tileToRight & (1 << (u32)TileWallDirectionBit::Left)))
				{
					vec2 oldTileRighttWallA = vec2{
						(float)oldBallCenterTileCoords.x * CachedTileSize + CachedTileSize,
						(float)oldBallCenterTileCoords.y * CachedTileSize
					};
					vec2 oldTileRightWallB = vec2{
						(float)oldBallCenterTileCoords.x * CachedTileSize + CachedTileSize,
						(float)oldBallCenterTileCoords.y * CachedTileSize + CachedTileSize
					};

					if (CollisionHelpers::isintersect(oldTileRighttWallA, oldTileRightWallB, oldballCenter, ballCenter))
					{
						hasCollided = true;
						collidedCell = cellToRight;
						collisionNormal = MovementHelpers::Left;
						break;
					}
				}
				if ((ballCenterTile & (1 << (u32)TileWallDirectionBit::Down)) || (tileToRight & (1 << (u32)TileWallDirectionBit::Up)))
				{
					vec2 newTileBottomWallA = vec2{
						(float)ballCenterTileCoords.x * CachedTileSize,
						(float)ballCenterTileCoords.y * CachedTileSize + CachedTileSize
					};
					vec2 newTileBottomWallB = vec2{
						(float)ballCenterTileCoords.x * CachedTileSize + CachedTileSize,
						(float)ballCenterTileCoords.y * CachedTileSize + CachedTileSize
					};
					if (CollisionHelpers::isintersect(newTileBottomWallA, newTileBottomWallB, oldballCenter, ballCenter))
					{
						hasCollided = true;
						collidedCell = ivec2{ (i32)ballCenterTileCoords.x,  (i32)ballCenterTileCoords.y };
						collisionNormal = MovementHelpers::Down;
						break;
					}
				}
				if ((ballCenterTile & (1 << (u32)TileWallDirectionBit::Left)) || (tileAbove & (1 << (u32)TileWallDirectionBit::Right)))
				{
					vec2 newTileLeftWallA = vec2{
						(float)ballCenterTileCoords.x * CachedTileSize,
						(float)ballCenterTileCoords.y * CachedTileSize
					};
					vec2 newTileLeftWallB = vec2{
						(float)ballCenterTileCoords.x * CachedTileSize ,
						(float)ballCenterTileCoords.y * CachedTileSize + CachedTileSize
					};
					if (CollisionHelpers::isintersect(newTileLeftWallA, newTileLeftWallB, oldballCenter, ballCenter))
					{
						hasCollided = true;
						collidedCell = ivec2{ (i32)ballCenterTileCoords.x,  (i32)ballCenterTileCoords.y };
						collisionNormal = MovementHelpers::Right;
						break;
					}
				}
			}
			break;
			case 0:
				if ((oldBallCenterTile & (1 << (u32)TileWallDirectionBit::Right)) || (ballCenterTile & (1 << (u32)TileWallDirectionBit::Left)))
				{
					// collision
					hasCollided = true;
					collidedCell = ivec2{ (i32)ballCenterTileCoords.x, (i32)ballCenterTileCoords.y };
					collisionNormal = MovementHelpers::Left;
				}
				break;
			case 1:
			{
				ivec2 cellToRight = ivec2{ (i32)oldBallCenterTileCoords.x, (i32)oldBallCenterTileCoords.y } + ivec2{ 1,0 };
				ivec2 cellBelow = ivec2{ (i32)oldBallCenterTileCoords.x, (i32)oldBallCenterTileCoords.y } + ivec2{ 0,-1 };

				u8 tileToRight = CachedTiledWorld->GetCellAtIndex(cellToRight);
				u8 tileBelow = CachedTiledWorld->GetCellAtIndex(cellBelow);
				if ((tileBelow & (1 << (u32)TileWallDirectionBit::Up)) || (oldBallCenterTile & (1 << (u32)TileWallDirectionBit::Down)))
				{
					vec2 oldTileBottomtWallA = vec2{
						(float)oldBallCenterTileCoords.x * CachedTileSize,
						(float)oldBallCenterTileCoords.y * CachedTileSize + CachedTileSize
					};
					vec2 oldTileBottomtWallB = vec2{
						(float)oldBallCenterTileCoords.x * CachedTileSize + CachedTileSize,
						(float)oldBallCenterTileCoords.y * CachedTileSize + CachedTileSize,
					};
					if (CollisionHelpers::isintersect(oldTileBottomtWallA, oldTileBottomtWallB, oldballCenter, ballCenter))
					{
						hasCollided = true;
						collidedCell = cellBelow;
						collisionNormal = MovementHelpers::Up;
						break;
					}
				}
				if ((oldBallCenterTile & (1 << (u32)TileWallDirectionBit::Right)) || (tileToRight & (1 << (u32)TileWallDirectionBit::Left)))
				{
					vec2 oldTileRighttWallA = vec2{
						(float)oldBallCenterTileCoords.x * CachedTileSize + CachedTileSize,
						(float)oldBallCenterTileCoords.y * CachedTileSize
					};
					vec2 oldTileRightWallB = vec2{
						(float)oldBallCenterTileCoords.x * CachedTileSize + CachedTileSize,
						(float)oldBallCenterTileCoords.y * CachedTileSize + CachedTileSize
					};

					if (CollisionHelpers::isintersect(oldTileRighttWallA, oldTileRightWallB, oldballCenter, ballCenter))
					{
						hasCollided = true;
						collidedCell = cellToRight;
						collisionNormal = MovementHelpers::Left;
						break;
					}
				}
				if ((ballCenterTile & (1 << (u32)TileWallDirectionBit::Down)) || (tileToRight & (1 << (u32)TileWallDirectionBit::Up)))
				{
					vec2 newTileBottomWallA = vec2{
						(float)ballCenterTileCoords.x * CachedTileSize,
						(float)ballCenterTileCoords.y * CachedTileSize + CachedTileSize
					};
					vec2 newTileBottomWallB = vec2{
						(float)ballCenterTileCoords.x * CachedTileSize + CachedTileSize,
						(float)ballCenterTileCoords.y * CachedTileSize + CachedTileSize
					};
					if (CollisionHelpers::isintersect(newTileBottomWallA, newTileBottomWallB, oldballCenter, ballCenter))
					{
						hasCollided = true;
						collidedCell = ivec2{ (i32)ballCenterTileCoords.x,  (i32)ballCenterTileCoords.y };
						collisionNormal = MovementHelpers::Down;
						break;
					}
				}
				if ((ballCenterTile & (1 << (u32)TileWallDirectionBit::Left)) || (tileBelow & (1 << (u32)TileWallDirectionBit::Right)))
				{
					vec2 newTileLeftWallA = vec2{
						(float)ballCenterTileCoords.x * CachedTileSize,
						(float)ballCenterTileCoords.y * CachedTileSize
					};
					vec2 newTileLeftWallB = vec2{
						(float)ballCenterTileCoords.x * CachedTileSize ,
						(float)ballCenterTileCoords.y * CachedTileSize + CachedTileSize
					};
					if (CollisionHelpers::isintersect(newTileLeftWallA, newTileLeftWallB, oldballCenter, ballCenter))
					{
						hasCollided = true;
						collidedCell = ivec2{ (i32)ballCenterTileCoords.x,  (i32)ballCenterTileCoords.y };
						collisionNormal = MovementHelpers::Right;
						break;
					}
				}
			}
			break;
			}
			break;
		}

		if (hasCollided)
		{
			vec2 collidedCellTL = vec2{ collidedCell.x * CachedTileSize, collidedCell.y * CachedTileSize };

			vec2 collisionEdge = ((collidedCellTL + vec2{ CachedTileSize / 2.0f, CachedTileSize / 2.0f }) + collisionNormal * (CachedTileSize / 2.0f));
			Position = oldPos;
			if (collisionNormal == MovementHelpers::Up || collisionNormal == MovementHelpers::Down)
			{
				DirectionVector.y *= -1;
			}
			else if (collisionNormal == MovementHelpers::Left || collisionNormal == MovementHelpers::Right)
			{
				DirectionVector.x *= -1;
			}
		}
		vec2 ballCenterNew = Position + vec2{ CachedTileSize / 2.0f, CachedTileSize / 2.0f };
		vec2 ownerPos = Owner->GetPosition();

		if (CollisionHelpers::CircleRect(ballCenter.x, ballCenter.y, (float)CrystalBallRadius, ownerPos.x, ownerPos.y, CachedTileSize, CachedTileSize))
		{
			Owner->CatchBall();
		}
	}
}
