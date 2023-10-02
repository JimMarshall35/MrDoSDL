#pragma once
#include <vector>
#include <string>
#include "SDL.h"

class IAnimationAssetManager
{
public:
	virtual void MakeAnimationRectFramesFromName(const std::string& animationName, std::vector<SDL_Rect>& outRectFrames) const = 0;
	virtual SDL_Surface* GetAnimationsSpriteSheetSurface() const = 0;
	virtual void MakeSingleSpriteRectFrame(const std::string& singleSpriteName, SDL_Rect& outRect) const = 0;
};