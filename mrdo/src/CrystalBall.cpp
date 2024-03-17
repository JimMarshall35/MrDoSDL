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
#include "GameFramework.h"
#include <iostream>
#include <algorithm>

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
	std::cout << "CrystalBall created\n";
}

CrystalBall::~CrystalBall()
{

	std::cout << "CrystalBall destroyed\n";
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
	State = CrystalBallState::CharacterCarrying;
}


void CrystalBall::UpdateActiveBallInternal(float deltaT)
{
	if (State != CrystalBallState::Released)
	{
		return;
	}
	struct Collision
	{
		vec2 N;
		float t;

	};
	std::vector<Collision> collisions;
	vec2 velocity = DirectionVector * CrystalBallSpeed * deltaT;

	float distanceToTravelLeft = velocity.Magnitude();

	while (distanceToTravelLeft > 0 && !CollisionHelpers::EqualWithEpsilon(distanceToTravelLeft, 0.0f))
	{
		vec2 oldballCenter = Position + vec2{ CachedTileSize / 2.0f, CachedTileSize / 2.0f };
		ivec2 oldBallCenterTileCoords = ivec2{ (u32)(oldballCenter.x / CachedTileSize), (u32)(oldballCenter.y / CachedTileSize)};

		vec2 newBallCenter = oldballCenter + velocity;
		ivec2 newBallCenterTileCoords = ivec2{ (u32)(oldballCenter.x / CachedTileSize), (u32)(oldballCenter.y / CachedTileSize) };

		ivec2 cellsOccupiedByBall[4] = {
		/* topLeft      */{ (oldballCenter.x - CrystalBallRadius) / CachedTileSize, (oldballCenter.y - CrystalBallRadius) / CachedTileSize },
		/* bottomRight  */{ (oldballCenter.x + CrystalBallRadius) / CachedTileSize, (oldballCenter.y + CrystalBallRadius) / CachedTileSize },
 		/* topLeft2     */{ (newBallCenter.x - CrystalBallRadius) / CachedTileSize, (newBallCenter.y - CrystalBallRadius) / CachedTileSize },
		/* bottomRight2 */{ (newBallCenter.x + CrystalBallRadius) / CachedTileSize, (newBallCenter.y + CrystalBallRadius) / CachedTileSize}
		};

		ivec2 tl, br;
		CollisionHelpers::CalcBoundingBox(tl, br, cellsOccupiedByBall, 4);
		std::vector<ivec2> cellsToTest;
		std::vector<std::pair<vec2, vec2>> lines;
		for (int y = tl.y; y <= br.y; y++)
		{
			for (int x = tl.x; x <= br.x; x++)
			{
				cellsToTest.emplace_back(x, y);
			}
		}

		CachedTiledWorld->GetLinesFromCells(lines, cellsToTest);
		const float distanceToTravel = velocity.Magnitude();
	
	
		float smallestDistance = std::numeric_limits<float>::max();
		collisions.clear();

		int closestCollisionIndex = -1;
		bool collisionOcurred = false;
		for (std::pair<vec2, vec2> line : lines)
		{
			
			vec2 N;
			float t;
			CollisionHelpers::circleLine(oldballCenter, CrystalBallRadius, line.first, line.second, DirectionVector * CrystalBallSpeed * deltaT, t, N);
			if (t >= 0)
			{
				collisions.push_back({ N,t });
			}
			
		}
		if (!collisions.empty())
		{
			std::sort(collisions.begin(), collisions.end(), [](const Collision& c1, const Collision& c2) {
				return c1.t < c2.t;
			});

			newBallCenter = oldballCenter + velocity * collisions[0].t;
			distanceToTravelLeft -= (oldballCenter - newBallCenter).Magnitude();
			DirectionVector = vec2::Reflect(DirectionVector, collisions[0].N).Normalized();;
			velocity = DirectionVector * CrystalBallSpeed * (distanceToTravelLeft / distanceToTravel) * deltaT;
		}
		else
		{
			distanceToTravelLeft = 0;
		}

		Position = newBallCenter - vec2{CachedTileSize / 2.0f, CachedTileSize / 2.0f};
		//printf("dfjiods");
		
		vec2 ownerPos = Owner->GetPosition();

		if (CollisionHelpers::CircleRect(newBallCenter.x, newBallCenter.y, (float)CrystalBallRadius, ownerPos.x, ownerPos.y, CachedTileSize, CachedTileSize))
		{
			Owner->CatchBall();
		}
		CachedEnemyManager->IterateActiveEnemies([&newBallCenter, this](Enemy& enemy) {
			if (CollisionHelpers::CircleRect(newBallCenter.x, newBallCenter.y, (float)CrystalBallRadius, enemy.Pos.x, enemy.Pos.y, CachedTileSize, CachedTileSize))
			{
				CachedEnemyManager->KillEnemy(&enemy);
				// should go into cooldown here 
				TriggerOnHitEnemyParticleEffect(enemy);
				EnemyDeath d;
				d.Reason = EnemyDeathReason::CrystalBall;
				d.NumberKilledTotal = 1;
				if (IsSignificantEnemyType(enemy.Type))
				{
					d.NumberSignificantKilled = 1;
				}
				GameFramework::SendFrameworkMessage<EnemyDeath>(d);
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