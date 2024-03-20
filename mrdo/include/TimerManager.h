#pragma once
#include <functional>

struct TimerCallback
{

};
class TimerManager
{
public:
	void Update(float deltaT);
	void RegisterTimerCallback();
};