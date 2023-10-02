#pragma once
#include <memory>
#include "IAnimationAssetManager.h"

class IConfigFile;
struct SDL_Surface;

class AnimationAssetManager : public IAnimationAssetManager
{
public:
	AnimationAssetManager(const std::shared_ptr<IConfigFile>& config, SDL_Surface* screenSurface);

	//IAnimationAssetManager
	virtual void MakeAnimationRectFramesFromName(const std::string& animationName, std::vector<SDL_Rect>& outRectFrames) const override;
	virtual void MakeSingleSpriteRectFrame(const std::string& singleSpriteName, SDL_Rect& outRect) const override;

	virtual SDL_Surface* GetAnimationsSpriteSheetSurface() const override { return AnimationsSpriteSheet; }

private:
	std::shared_ptr<IConfigFile> Config;
	SDL_Surface* AnimationsSpriteSheet;
};