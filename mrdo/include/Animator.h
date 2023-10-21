#pragma once
#include <vector>
#include "SDL.h"
#include "CommonTypedefs.h"

struct Animator
{
	float AccumulatedTime = 0;
	float FPS;
	u32 OnAnimFrame = 0;
	std::vector<SDL_Rect>* CurrentAnimation;
	bool bIsAnimating = true;
	bool bLooping = true;
	bool bFinished = false;
	void Update(float deltaT);
	const SDL_Rect& GetCurrentFrame() const;
};