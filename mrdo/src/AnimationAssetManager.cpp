#include "AnimationAssetManager.h"
#include "IConfigFile.h"
#include <cassert>
#include "SDL.h"

AnimationAssetManager::AnimationAssetManager(const std::shared_ptr<IConfigFile>& config, SDL_Surface* screenSurface)
	:Config(config),
	AnimationsSpriteSheet(SDL_LoadBMP(config->GetAnimationsConfigData().SpriteSheetAssetPath.c_str()))
{
	assert(AnimationsSpriteSheet);
	
	// ensure loaded bmp is the same format as the screen
	AnimationsSpriteSheet = SDL_ConvertSurface(AnimationsSpriteSheet, screenSurface->format);

	// set colour of pixel to be treated as transparent when blitting
	const AnimationsConfigData& data = Config->GetAnimationsConfigData();
	SDL_SetSurfaceColorKey(AnimationsSpriteSheet, SDL_TRUE, SDL_MapRGB(screenSurface->format, data.ColourKeyR, data.ColourKeyG, data.ColourKeyB));
}

void AnimationAssetManager::MakeAnimationRectFramesFromName(const std::string& animationName, std::vector<SDL_Rect>& outRectFrames) const
{
	const AnimationsConfigData& animData = Config->GetAnimationsConfigData();
	assert(animData.Animations.find(animationName) != animData.Animations.end());
	const std::vector<uvec2>& frames = animData.Animations.at(animationName);
	outRectFrames.clear();
	for (const uvec2& frame : frames)
	{
		SDL_Rect rect;
		rect.x = frame.x * animData.TileSize;
		rect.y = frame.y * animData.TileSize;
		rect.w = animData.TileSize;
		rect.h = animData.TileSize;
		outRectFrames.push_back(rect);
	}
}

void AnimationAssetManager::MakeSingleSpriteRectFrame(const std::string& singleSpriteName, SDL_Rect& outRect) const
{
	const AnimationsConfigData& animData = Config->GetAnimationsConfigData();
	assert(animData.SingleSprites.find(singleSpriteName) != animData.SingleSprites.end());
	uvec2 spriteSheetCoords = animData.SingleSprites.at(singleSpriteName);
	outRect.x = spriteSheetCoords.x * animData.TileSize;
	outRect.y = spriteSheetCoords.y * animData.TileSize;
	outRect.w = animData.TileSize;
	outRect.h = animData.TileSize;
}
