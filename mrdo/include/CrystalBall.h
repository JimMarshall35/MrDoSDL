#pragma once
#include "VectorTypes.h"
#include "SDL.h"
#include "CommonTypedefs.h"
#include <memory>

class Character;
class IAnimationAssetManager;
class IConfigFile;
class TiledWorld;
class EnemyManager;
struct Enemy;

#define NUM_PARTICLES 8

class CrystalBall
{
private:
	enum class CrystalBallState
	{
		CharacterCarrying,
		Released,
		ParticleEffectOnHitEnemy,
		Cooldown,
		ParticleEffectOnRegenerate,
		NumCrystalBallStates
	};
public:
	CrystalBall(Character* owner, IAnimationAssetManager* anim, IConfigFile* configFile, TiledWorld* tiledWorld, EnemyManager* enemyManager);
	void Draw(SDL_Surface* windowSurface, float scale) const;
	void Release();
	void Update(float deltaT);
	bool IsReleased() const { return State != CrystalBallState::CharacterCarrying; };
	void OnCaught();
	void ResetStateOnDeath();

private:
	void InitialiseParticleDirectionsAndSpeeds();
	void InitialiseRegenerateTimeTable();
	void UpdateActiveBallInternal(float deltaT);
	void UpdateOnHitParticleEffect(float deltaT);
	void UpdateRegenerateParticleEffect(float deltaT);
	void TriggerOnHitEnemyParticleEffect(const Enemy& enemy);
	void TriggerOnRegenerateParticleEffect();
	void SetCooldownTimer();
	void UpdateCooldownState(float deltaT);

private:
	Character* Owner;
	vec2 Position;
	vec2 DirectionVector;
	IAnimationAssetManager* AnimationAssetManager;
	IConfigFile* ConfigFile;
	SDL_Rect BounceUpSprite;
	SDL_Rect BounceDownSprite;
	SDL_Rect BallParticleSprite;
	float CrystalBallSpeed;
	float CachedTileSize;
	TiledWorld* CachedTiledWorld;
	u32 CrystalBallRadius;
	u32 NumUpdatesPerFrame;
	EnemyManager* CachedEnemyManager;
	CrystalBallState State;

	float OnHitParticleEffectDuration;
	float OnRegenerateParticleEffectDuration;
	float OnHitParticleEffectRadius;
	float OnRegenerateParticleEffectRadius;

	vec2 Particles[NUM_PARTICLES];
	vec2 ParticleDirections[NUM_PARTICLES];
	float ParticleSpeeds[NUM_PARTICLES];
	float Timer = 0;
	size_t RegenerateTimeTableSize = 0;
	size_t RegenerateTimeIndex = 0;
	std::unique_ptr<float[]> RegenerateTimeTable;
};