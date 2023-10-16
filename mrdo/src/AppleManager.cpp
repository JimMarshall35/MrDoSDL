#include "AppleManager.h"
#include "IConfigFile.h"
#include "Event.h"
#include <iostream>
#include <cassert>
#include "IAnimationAssetManager.h"
#include "Character.h"
#include "CollisionHelpers.h"
#include "TiledWorld.h"


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
	CachedSpriteDims(vec2{ (float)configFile->GetAnimationsConfigData().TileSize, (float)configFile->GetAnimationsConfigData().TileSize }),
	PushedAppleStack(std::make_unique<Apple*[]>(ApplePoolSize)),
	CachedTiledWorld(tiledWorld),
	AppleSlideSpeed(configFile->GetFloatValue("AppleSlideSpeed")),
	AppleFallSpeed(configFile->GetFloatValue("AppleFallSpeed")),
	AppleWobbleTime(configFile->GetFloatValue("AppleWobbleTime")),
	AppleSplitTime(configFile->GetFloatValue("AppleSplitTime"))
{
	onNewLevelStarted += &LOnNewLevelStarted;
	CachedBackgroundTileSize = configFile->GetBackgroundConfigData().TileSize;
	assetManager->MakeAnimationRectFramesFromName("Apple_Wobble", WobbleAnimation);
	assetManager->MakeAnimationRectFramesFromName("Apple_Split_LeftHalf", LeftHalfSplitAnimation);
	assetManager->MakeAnimationRectFramesFromName("Apple_Split_RightHalf", RightHalfSplitAnimation);
	assert(LeftHalfSplitAnimation.size() == RightHalfSplitAnimation.size());
	assert(LeftHalfSplitAnimation.size() == 3);
	assert(WobbleAnimation.size() == 3);
}

void AppleManager::Update(float deltaT)
{
	for (int i = 0; i < ThisLevelNumApplesAtStart; i++)
	{
		Apple& apple = ApplePool[i];
		if (apple.State != AppleState::Inactive)
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
		if (apple.State != AppleState::Inactive)
		{
			switch (apple.State)
			{
			case AppleState::Falling:
			case AppleState::Settled:
				{
					SDL_Rect dst;
					dst.w = CachedBackgroundTileSize * scale;
					dst.h = CachedBackgroundTileSize * scale;
					dst.x = (float)apple.Position.x * scale;
					dst.y = (float)apple.Position.y * scale;

					const SDL_Rect& rect = WobbleAnimation[1];
					SDL_BlitSurfaceScaled(surface, &rect, windowSurface, &dst);
				}
				break;
			case AppleState::Sliding:
			case AppleState::Wobbling:
				{
					SDL_Rect dst;
					dst.w = CachedBackgroundTileSize * scale;
					dst.h = CachedBackgroundTileSize * scale;
					dst.x = (float)apple.Position.x * scale;
					dst.y = (float)apple.Position.y * scale;

					const SDL_Rect& rect = WobbleAnimation[apple.OnAnimationFrame];
					SDL_BlitSurfaceScaled(surface, &rect, windowSurface, &dst);
				}
				break;
			case AppleState::Splitting:
				{
					SDL_Rect leftHalfDst;
					SDL_Rect rightHalfDst;
					leftHalfDst.x = (float)apple.Position.x * scale - ((float)CachedSpriteDims.x / 2.0f) * scale;
					leftHalfDst.y = apple.Position.y * scale;
					leftHalfDst.w = CachedSpriteDims.x * scale;
					leftHalfDst.h = CachedSpriteDims.y * scale;

					rightHalfDst.x = (float)apple.Position.x * scale + ((float)CachedSpriteDims.x / 2.0f) * scale;
					rightHalfDst.y = apple.Position.y * scale;
					rightHalfDst.w = CachedSpriteDims.x * scale;
					rightHalfDst.h = CachedSpriteDims.y * scale;
				
					const SDL_Rect& lRect = LeftHalfSplitAnimation[apple.OnAnimationFrame];
					SDL_BlitSurfaceScaled(surface, &lRect, windowSurface, &leftHalfDst);
					const SDL_Rect& rRect = RightHalfSplitAnimation[apple.OnAnimationFrame];
					SDL_BlitSurfaceScaled(surface, &rRect, windowSurface, &rightHalfDst);
				}
				break;
			}
			
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
		if (apple.State != AppleState::Inactive)
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
				bool pushing = false;
				if ((apple.Position.x > characterPos.x) && (apple.Position.y == characterPos.y))
				{
					// mr do is approaching from the left therefore pushing the apple - resolve collision
					apple.Position.x = characterPos.x + CachedSpriteDims.x;
					pushing = true;
				}
				else if ((apple.Position.x < characterPos.x) && (apple.Position.y == characterPos.y))
				{
					// mr do is approaching from the right therefore pushing the apple - resolve collision
					apple.Position.x = characterPos.x - CachedSpriteDims.x;
					pushing = true;
				}

				if (pushing)
				{
					PushedAppleStackSize = 0;
					PushedAppleStack[PushedAppleStackSize++] = &apple;

					RecursivelyPushApples(apple);
					ClampPushedApples();
				
					Apple* lastApple = PushedAppleStack[PushedAppleStackSize - 1];

					if (IsCellBelowEmpty(lastApple))
					{
						lastApple->State = AppleState::Sliding;
						float lastAppleCenterX = lastApple->Position.x + CachedSpriteDims.x / 2.0f;
						int cellBelowCoordsX = lastAppleCenterX / CachedBackgroundTileSize;
						lastApple->SlideDestination = cellBelowCoordsX * CachedBackgroundTileSize;
					}
				}
			
			}
			else if(IsCellBelowEmpty(&apple))
			{
				apple.State = AppleState::Wobbling;
			}
			// todo: check here to see if apple should transition to the falling state and make transition if required
		}
		break;
	case AppleState::Falling:
		// possible transiitons: Settled, Spliiting
		{
			float fallAmountThisFrame = 1.0f * AppleFallSpeed * deltaT;
			apple.Position.y += fallAmountThisFrame;
			apple.DistanceFallen += fallAmountThisFrame;
			if (!IsCellDirectlyBelowEmpty(&apple))
			{
				if (apple.DistanceFallen <= CachedBackgroundTileSize + CachedBackgroundTileSize * 0.5f)
				{
					vec2 appleCenter = apple.Position + vec2{ CachedSpriteDims.x / 2.0f, CachedSpriteDims.y / 2.0f };
					uvec2 appleCoords = { appleCenter.x / CachedBackgroundTileSize, appleCenter.y / CachedBackgroundTileSize };
					apple.Position = { appleCoords.x * CachedBackgroundTileSize, appleCoords.y * CachedBackgroundTileSize };
					apple.DistanceFallen = 0.0f;
					apple.State = AppleState::Settled;
				}
				else
				{
					apple.AnimationTimer = 0.0f;
					apple.OnAnimationFrame = 0.0f;
					apple.State = AppleState::Splitting;
				}
				
			}
		}
		break;
	case AppleState::Splitting:
		// possible transition: None. Apple becomes inactive after animation
		{
			apple.AnimationTimer += deltaT;
			apple.OnAnimationFrame = (int)((apple.AnimationTimer / AppleSplitTime) * LeftHalfSplitAnimation.size());
			if (apple.AnimationTimer >= AppleSplitTime)
			{
				apple.State = AppleState::Inactive;
			}
		}
		break;
	case AppleState::Wobbling:
		// possible transition: falling
		{
			static const u32 wobblePatternSize = 5;
			static const u32 wobblePattern[5] = { 1, 0, 1, 2, 1 };
			apple.AnimationTimer += deltaT;
			apple.OnAnimationFrame = wobblePattern[(int)((apple.AnimationTimer / AppleWobbleTime) * wobblePatternSize)];
			if (apple.AnimationTimer >= AppleWobbleTime)
			{
				apple.AnimationTimer = 0;
				apple.DistanceFallen = 0.0f;
				apple.State = AppleState::Falling;
			}
		}
		break;
	case AppleState::Sliding:
		// possible transition: falling
		{
			float movmentdir = apple.SlideDestination - apple.Position.x > 0 ? 1.0f : -1.0f;
			if (movmentdir > 0)
			{
				apple.OnAnimationFrame = 0;
			}
			else
			{
				apple.OnAnimationFrame = 2;
			}
			apple.Position.x += movmentdir * AppleSlideSpeed * deltaT;
			float newMovmentdir = apple.SlideDestination - apple.Position.x > 0 ? 1.0f : -1.0f;
			if (newMovmentdir != movmentdir)
			{
				apple.Position.x = apple.SlideDestination;
				apple.OnAnimationFrame = 0;
				apple.AnimationTimer = 0;
				apple.DistanceFallen = 0.0f;
				apple.State = AppleState::Falling;
			}
		}
		break;
	}
}

void AppleManager::RecursivelyPushApples(Apple& apple)
{
	for (int i = 0; i < ThisLevelNumApplesAtStart; i++)
	{
		Apple& otherApple = ApplePool[i];
		if ((apple.State != AppleState::Inactive) && (&otherApple != &apple) && (otherApple.State != AppleState::Inactive) && (otherApple.Position.y == apple.Position.y))
		{
			if (CollisionHelpers::AABBCollision(
				apple.Position,
				otherApple.Position,
				CachedSpriteDims,
				CachedSpriteDims))
			{
				if (apple.Position.x < otherApple.Position.x)
				{
					otherApple.Position.x = apple.Position.x + CachedSpriteDims.x + A_SMALL_NUMBER;
				}
				else
				{
					otherApple.Position.x = apple.Position.x - CachedSpriteDims.x - A_SMALL_NUMBER;
				}
				PushedAppleStack[PushedAppleStackSize++] = &otherApple;

				RecursivelyPushApples(otherApple);
			}
		}
	}
}

void AppleManager::ClampPushedApples()
{
	assert(PushedAppleStackSize);
	vec2 characterPos = CharacterRef->GetPosition();
	Apple* endApple = PushedAppleStack[PushedAppleStackSize - 1];
	float tileMaxX = (CachedLevelSize.x - 1) * CachedBackgroundTileSize;

	// does the end epple in the stack need to be clamped
	if (endApple->Position.x > tileMaxX || endApple->Position.x < 0)
	{
		// clamp the last apple being pushed to the edges of the screen
		if (endApple->Position.x > tileMaxX)
		{
			endApple->Position.x = tileMaxX;
		}
		else if (endApple->Position.x < 0)
		{
			endApple->Position.x = 0;
		}

		// propagate collision resolution to all the other apples
		for (int i = PushedAppleStackSize - 2; i >= 0; i--)
		{
			Apple* apple = PushedAppleStack[i];
			Apple* previousApple = PushedAppleStack[i + 1];
			if (apple->Position.x < previousApple->Position.x)
			{
				apple->Position.x = previousApple->Position.x - CachedSpriteDims.x;
			}
			else
			{
				apple->Position.x = previousApple->Position.x + CachedSpriteDims.x;
			}
		}

		// propagate collision resolution to mr do
		Apple* firstApple = PushedAppleStack[0];
		if (characterPos.x < firstApple->Position.x)
		{
			CharacterRef->SetPosition({ firstApple->Position.x - CachedSpriteDims.x, characterPos.y });
		}
		else
		{
			CharacterRef->SetPosition({ firstApple->Position.x + CachedSpriteDims.x, characterPos.y });
		}
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
		ApplePool[i].State = AppleState::Settled;
		ApplePool[i].Position = vec2{ (float)startedLevel.Apples[i].x * CachedBackgroundTileSize, (float)startedLevel.Apples[i].y * CachedBackgroundTileSize };
	}
	CachedLevelSize.x = startedLevel.NumCols;
	CachedLevelSize.y = startedLevel.NumRows;
}

bool AppleManager::IsCellBelowEmpty(Apple* apple) const
{
	float lastAppleCenterX = apple->Position.x + CachedSpriteDims.x / 2.0f;
	float lastAppleCenterY = apple->Position.y + CachedSpriteDims.y / 2.0f;
	float cellBelowY = lastAppleCenterY + CachedBackgroundTileSize;
	int cellBelowCoordsX = lastAppleCenterX / CachedBackgroundTileSize;
	int cellBelowCorrdsY = cellBelowY / CachedBackgroundTileSize;
	u8 cell = CachedTiledWorld->GetCellAtIndexValue({ cellBelowCoordsX, cellBelowCorrdsY });
	return (cell & (1 << (u8)TileWallDirectionBit::Center)) == 0;
}

bool AppleManager::IsCellDirectlyBelowEmpty(Apple* apple) const
{
	float lastAppleCenterX = apple->Position.x + CachedSpriteDims.x / 2.0f;
	float lastAppleCenterY = apple->Position.y + CachedSpriteDims.y / 2.0f;
	float cellBelowY = lastAppleCenterY + CachedBackgroundTileSize / 2.0f + A_SMALL_NUMBER;
	int cellBelowCoordsX = lastAppleCenterX / CachedBackgroundTileSize;
	int cellBelowCorrdsY = cellBelowY / CachedBackgroundTileSize;
	u8 cell = CachedTiledWorld->GetCellAtIndexValue({ cellBelowCoordsX, cellBelowCorrdsY });
	return (cell & (1 << (u8)TileWallDirectionBit::Center)) == 0;
}