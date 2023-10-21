#include "Animator.h"

void Animator::Update(float deltaT)
{
	if (!bLooping && bFinished)
	{
		return;
	}
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
			
			if (OnAnimFrame >= CurrentAnimation->size()) 
			{
				if (bLooping)
				{
					OnAnimFrame = 0;
				}
				else
				{
					--OnAnimFrame;
					bFinished = true;
				}
			}
		}
	}
}

const SDL_Rect& Animator::GetCurrentFrame() const
{
	return (*CurrentAnimation)[OnAnimFrame];
}
