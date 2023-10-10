#include "AppleManager.h"
#include "IConfigFile.h"
#include "Event.h"
#include <iostream>
#include <cassert>
#include "IAnimationAssetManager.h"
#include "Character.h"
#include "CollisionHelpers.h"


AppleManager::AppleManager(
	const std::shared_ptr<IAnimationAssetManager>& assetManager,
	const std::shared_ptr<IConfigFile>& configFile,
	const std::shared_ptr<TiledWorld>& tiledWorld,
	Event<int>& onNewLevelStarted
)
	:CachedConfig(configFile),
	AnimationAssetManager(assetManager),
	ApplePoolSize(configFile->GetUIntValue("ApplePoolSize")),
	ThisLevelNumApplesAtStart(0),
	ApplePool(std::make_unique<Apple[]>(ApplePoolSize)),
	LOnNewLevelStarted(this),
	CachedSpriteDims(vec2{ (float)configFile->GetAnimationsConfigData().TileSize, (float)configFile->GetAnimationsConfigData().TileSize })
{
	onNewLevelStarted += &LOnNewLevelStarted;
	CachedBackgroundTileSize = configFile->GetBackgroundConfigData().TileSize;
	assetManager->MakeAnimationRectFramesFromName("Apple_Wobble", WobbleAnimation);
}

void AppleManager::Update(float deltaT)
{
	for (int i = 0; i < ThisLevelNumApplesAtStart; i++)
	{
		Apple& apple = ApplePool[i];
		if (apple.bIsActive)
		{
			UpdateSingleApple(deltaT, apple);
		}
	}
}

void AppleManager::Draw(SDL_Surface* windowSurface, float scale) const
{
	SDL_Surface* surface = AnimationAssetManager->GetAnimationsSpriteSheetSurface();
	for (int i = 0; i < ThisLevelNumApplesAtStart; i++)
	{
		const Apple& apple = ApplePool[i];
		if (apple.bIsActive)
		{
			SDL_Rect dst;
			dst.w = CachedBackgroundTileSize * scale;
			dst.h = CachedBackgroundTileSize * scale;
			dst.x = (float)apple.Position.x * scale;
			dst.y = (float)apple.Position.y * scale;

			const SDL_Rect& rect = WobbleAnimation[1];//Animator.GetCurrentFrame();
			SDL_BlitSurfaceScaled(surface, &rect, windowSurface, &dst);
		}
	}
}

void AppleManager::SetCharacter(Character* character)
{
	CharacterRef = character;
}

AppleManager::Apple* AppleManager::FindActiveAppleByPredicate(std::function<bool(const Apple&)> predicate)
{
	for (int i = 0; i < ThisLevelNumApplesAtStart; i++)
	{
		// just iterate through all the apples even non active ones.
		// Could maintain link field to next active apple to minimise iterations 
		// but I don't think its worth it given anticpiated number of apples.
		// Also if I did this it would still have to initially iterate through all of them
		const Apple& apple = ApplePool[i];
		if (apple.bIsActive)
		{
			if (predicate(apple))
			{
				return &ApplePool[i];
			}
		}
	}
	return nullptr;
}

void AppleManager::UpdateSingleApple(float deltaT, Apple& apple)
{
	switch (apple.State)
	{
	case AppleState::Settled:
	{
		// possible transitions: falling, wobbling
		vec2 characterPos = CharacterRef->GetPosition();
		if (CollisionHelpers::AABBCollision(
			apple.Position + vec2{ 0, A_SMALL_NUMBER / 2.0f },
			characterPos,
			CachedSpriteDims - vec2{ 0, A_SMALL_NUMBER }, // take a small number from either side of our dims so we don't collide with apples to the left and right of us
			CachedSpriteDims))
		{
			if ((apple.Position.x > characterPos.x) && (apple.Position.y == characterPos.y))
			{
				// mr do is approaching from the left therefore pushing the apple - resolve collision
				apple.Position.x = characterPos.x + CachedSpriteDims.x;
			}
			else if ((apple.Position.x < characterPos.x) && (apple.Position.y == characterPos.y))
			{
				// mr do is approaching from the right therefore pushing the apple - resolve collision
				apple.Position.x = characterPos.x - CachedSpriteDims.x;
			}
			// todo - check in if statements above if apple is colliding with any other apples and push those too if it is
		}
		// todo: check here to see if apple should transition to the falling state and make transition if required
	}
		
		break;
	case AppleState::Falling:
		// possible transiitons: Settled, Spliiting
		break;
	case AppleState::Splitting:
		// possible transition: None. Apple becomes inactive after animation
		break;
	case AppleState::Wobbling:
		// possible transition: falling
		break;
	}
}

void AppleManager::OnNewLevelStarted(int levelNumber)
{
	const std::vector<LevelConfigData>& levels = CachedConfig->GetLevelsConfigData();
	const LevelConfigData& startedLevel = levels[levelNumber];
	ThisLevelNumApplesAtStart = startedLevel.Apples.size();
	assert(ThisLevelNumApplesAtStart <= ApplePoolSize);
	for (int i = 0; i < ThisLevelNumApplesAtStart; i++)
	{
		ApplePool[i].bIsActive = true;
		ApplePool[i].Position = vec2{ (float)startedLevel.Apples[i].x * CachedBackgroundTileSize, (float)startedLevel.Apples[i].y * CachedBackgroundTileSize };
	}
	std::cerr << "started\n";
}
