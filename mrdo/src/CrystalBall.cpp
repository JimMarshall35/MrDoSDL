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
#include "EnemyManager.h"

CrystalBall::CrystalBall(Character* owner, IAnimationAssetManager* anim, IConfigFile* configFile, TiledWorld* tiledWorld, EnemyManager* enemyManager)
	:Owner(owner),
	Position({0,0}),
	AnimationAssetManager(anim),
	ConfigFile(configFile),
	CachedTileSize(ConfigFile->GetBackgroundConfigData().TileSize),
	CachedTiledWorld(tiledWorld),
	CachedEnemyManager(enemyManager),
	State(CrystalBallState::CharacterCarrying)
{
	AnimationAssetManager->MakeSingleSpriteRectFrame("CrystalBall_BounceUp", BounceUpSprite);
	AnimationAssetManager->MakeSingleSpriteRectFrame("CrystalBall_BounceDown", BounceDownSprite);
	AnimationAssetManager->MakeSingleSpriteRectFrame("CrystalBall_Particle", BallParticleSprite);
	CrystalBallSpeed = ConfigFile->GetFloatValue("CrystalBallSpeed");
	CrystalBallRadius = ConfigFile->GetUIntValue("CrystalBallRadiusPixels");
	NumUpdatesPerFrame = ConfigFile->GetUIntValue("NumCrystalBallUpdates");
	OnHitParticleEffectDuration = ConfigFile->GetFloatValue("OnHitParticleEffectDuration");
	OnRegenerateParticleEffectDuration = ConfigFile->GetFloatValue("OnRegenerateParticleEffectDuration");
	OnHitParticleEffectRadius = ConfigFile->GetFloatValue("OnHitParticleEffectRadius");
	OnRegenerateParticleEffectRadius = ConfigFile->GetFloatValue("OnRegenerateParticleEffectRadius");
	InitialiseParticleDirectionsAndSpeeds();
	InitialiseRegenerateTimeTable();
}

void CrystalBall::Draw(SDL_Surface* windowSurface, float scale) const
{
	SDL_Surface* surface = AnimationAssetManager->GetAnimationsSpriteSheetSurface();
	SDL_Rect dst;
	float tileSize = ConfigFile->GetBackgroundConfigData().TileSize;
	const SDL_Rect* rectToDraw = nullptr;
	vec2 character = Owner->GetPosition();

	switch(State)
	{
	case CrystalBallState::Released:
		{
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
			
			dst.w = tileSize * scale;
			dst.h = tileSize * scale;
			dst.x = Position.x * scale;
			dst.y = Position.y * scale;
			SDL_BlitSurfaceScaled(surface, rectToDraw, windowSurface, &dst);
		}
		break;
	case CrystalBallState::ParticleEffectOnHitEnemy:
		for (int i = 0; i < NUM_PARTICLES; i++)
		{
			rectToDraw = &BallParticleSprite;
			const vec2& particle = Particles[i];
			dst.w = tileSize * scale;
			dst.h = tileSize * scale;
			dst.x = particle.x * scale;
			dst.y = particle.y * scale;
			SDL_BlitSurfaceScaled(surface, rectToDraw, windowSurface, &dst);
		}
	case CrystalBallState::ParticleEffectOnRegenerate:
		for (int i = 0; i < NUM_PARTICLES; i++)
		{
			rectToDraw = &BallParticleSprite;
			const vec2& particle = Particles[i];
			dst.w = tileSize * scale;
			dst.h = tileSize * scale;
			dst.x = (character.x + particle.x) * scale;
			dst.y = (character.y + particle.y) * scale;
			SDL_BlitSurfaceScaled(surface, rectToDraw, windowSurface, &dst);
		}
		break;
	}
}

void CrystalBall::Release()
{
	assert(State != CrystalBallState::Released);
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
	State = CrystalBallState::Released;
}

void CrystalBall::Update(float deltaT)
{
	switch (State)
	{
	case CrystalBallState::Released:
		for (int i = 0; i < NumUpdatesPerFrame; i++)
		{
			UpdateActiveBallInternal(deltaT / NumUpdatesPerFrame);
		}
		break;
	case CrystalBallState::ParticleEffectOnHitEnemy:
		UpdateOnHitParticleEffect(deltaT);
		break;
	case CrystalBallState::Cooldown:
		UpdateCooldownState(deltaT);
		break;
	case CrystalBallState::ParticleEffectOnRegenerate:
		UpdateRegenerateParticleEffect(deltaT);
		break;
	}
	
}

void CrystalBall::UpdateRegenerateParticleEffect(float deltaT)
{
	for (int i = 0; i < NUM_PARTICLES; i++)
	{
		vec2& particle = Particles[i];
		particle += (ParticleDirections[i] * ParticleSpeeds[i] * -deltaT);
	}
	Timer += deltaT;
	if (Timer >= OnHitParticleEffectDuration)
	{
		Timer = 0.0f;
		Owner->CatchBall();
	}
}

void CrystalBall::UpdateCooldownState(float deltaT)
{
	Timer -= deltaT;
	if (Timer <= 0.0f)
	{
		TriggerOnRegenerateParticleEffect();
		Timer = 0.0f;
	}
}

void CrystalBall::OnCaught()
{
	assert(State != CrystalBallState::CharacterCarrying);
	State = CrystalBallState::CharacterCarrying;
}

void CrystalBall::UpdateActiveBallInternal(float deltaT)
{
	if (State != CrystalBallState::Released)
	{
		return;
	}

	vec2 oldballCenter = Position + vec2{ CachedTileSize / 2.0f, CachedTileSize / 2.0f };
	vec2 oldBallCenter1By1TileIndex = vec2{ oldballCenter.x / CachedTileSize, oldballCenter.y / CachedTileSize };
	ivec2 oldBallCenterTileCoords = ivec2{ (u32)oldBallCenter1By1TileIndex.x, (u32)oldBallCenter1By1TileIndex.y };
	vec2 oldPos = Position;
	Position += (DirectionVector * CrystalBallSpeed * deltaT);

	vec2 ballCenter = Position + vec2{ CachedTileSize / 2.0f, CachedTileSize / 2.0f };
	vec2 ballCenter1By1TileIndex = vec2{ ballCenter.x / CachedTileSize, ballCenter.y / CachedTileSize };
	ivec2 ballCenterTileCoords = ivec2{ (u32)ballCenter1By1TileIndex.x, (u32)ballCenter1By1TileIndex.y };

	if (ballCenterTileCoords != oldBallCenterTileCoords)
	{
		u8 oldBallCenterTile = CachedTiledWorld->GetCellAtIndexValue(ivec2{ (i32)oldBallCenterTileCoords.x, (i32)oldBallCenter1By1TileIndex.y });
		u8 ballCenterTile = CachedTiledWorld->GetCellAtIndexValue(ivec2{ (i32)ballCenterTileCoords.x, (i32)ballCenterTileCoords.y });

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

				u8 tileToLeft = CachedTiledWorld->GetCellAtIndexValue(cellToLeft);
				u8 tileAbove = CachedTiledWorld->GetCellAtIndexValue(cellAbove);
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

				u8 tileToLeft = CachedTiledWorld->GetCellAtIndexValue(cellToLeft);
				u8 tileBelow = CachedTiledWorld->GetCellAtIndexValue(cellBelow);
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

				u8 tileToRight = CachedTiledWorld->GetCellAtIndexValue(cellToRight);
				u8 tileAbove = CachedTiledWorld->GetCellAtIndexValue(cellAbove);
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

				u8 tileToRight = CachedTiledWorld->GetCellAtIndexValue(cellToRight);
				u8 tileBelow = CachedTiledWorld->GetCellAtIndexValue(cellBelow);
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
		CachedEnemyManager->IterateActiveEnemies([&ballCenter, this](Enemy& enemy) {
			if (CollisionHelpers::CircleRect(ballCenter.x, ballCenter.y, (float)CrystalBallRadius, enemy.Pos.x, enemy.Pos.y, CachedTileSize, CachedTileSize))
			{
				CachedEnemyManager->KillEnemy(&enemy);
				// should go into cooldown here 
				//Owner->CatchBall(true);
				TriggerOnHitEnemyParticleEffect(enemy);
			}

		});
	}
}

void CrystalBall::UpdateOnHitParticleEffect(float deltaT)
{
	for (int i = 0; i < NUM_PARTICLES; i++)
	{
		vec2& particle = Particles[i];
		particle += ParticleDirections[i] * ParticleSpeeds[i] * deltaT;
	}
	Timer += deltaT;
	if (Timer >= OnHitParticleEffectDuration)
	{
		Timer = 0.0f;
		State = CrystalBallState::Cooldown;
		SetCooldownTimer();
	}
}

void CrystalBall::SetCooldownTimer()
{
	if (RegenerateTimeIndex + 1 == RegenerateTimeTableSize)
	{
		Timer = RegenerateTimeTable[RegenerateTimeIndex];
	}
	else
	{
		Timer = RegenerateTimeTable[RegenerateTimeIndex++];
	}
}

void CrystalBall::InitialiseParticleDirectionsAndSpeeds()
{
	float speed = OnHitParticleEffectRadius / OnHitParticleEffectDuration;
	vec2 direction = {0,-1};
	for (int i = 0; i < NUM_PARTICLES; i++)
	{
		ParticleDirections[i] = direction.Rotated((float(i) / float(NUM_PARTICLES))*360.0f).Normalized();
		ParticleSpeeds[i] = i % 2 ? speed : speed / 2.0f;
	}
}

void CrystalBall::TriggerOnHitEnemyParticleEffect(const Enemy& enemy)
{
	for (int i = 0; i < NUM_PARTICLES; i++)
	{
		Particles[i] = enemy.Pos;
	}
	State = CrystalBallState::ParticleEffectOnHitEnemy;
	Timer = 0.0f;
}

void CrystalBall::TriggerOnRegenerateParticleEffect()
{
	for (int i = 0; i < NUM_PARTICLES; i++)
	{
		Particles[i] = ParticleDirections[i] * (ParticleSpeeds[i] * OnRegenerateParticleEffectDuration);
	}
	State = CrystalBallState::ParticleEffectOnRegenerate;
	Timer = 0.0f;
}

void CrystalBall::InitialiseRegenerateTimeTable()
{
	int tableSize = ConfigFile->GetArraySize("CrystalBallCooldownTable");
	RegenerateTimeTable = std::make_unique<float[]>(tableSize);
	RegenerateTimeTableSize = tableSize;
	assert(tableSize > 0);
	for (int i = 0; i < tableSize; i++)
	{
		RegenerateTimeTable[i] = ConfigFile->GetFloatArrayValue("CrystalBallCooldownTable", i);
	}
}

void CrystalBall::ResetStateOnDeath()
{
	State = CrystalBallState::CharacterCarrying;
	RegenerateTimeIndex = 0;
	Timer = 0;
}