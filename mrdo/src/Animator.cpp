#include "Animator.h"

void Animator::Update(float deltaT)
{
	if (!bIsAnimating)
	{
		OnAnimFrame = 0;
	}
	else
	{
		AccumulatedTime += deltaT;
		if (AccumulatedTime >= 1.0f / FPS)
		{
			AccumulatedTime = 0.0f;
			++OnAnimFrame;
			if (OnAnimFrame >= CurrentAnimation->size()) {
				OnAnimFrame = 0;
			}
		}
	}
}

const SDL_Rect& Animator::GetCurrentFrame() const
{
	return (*CurrentAnimation)[OnAnimFrame];
}
