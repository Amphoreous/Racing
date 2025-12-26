// Simple timer class for measuring elapsed time

#include "Timer.h"

#include "raylib.h"

Timer::Timer()
{
	Start();
}

void Timer::Start()
{
	started_at = GetTime();
}

double Timer::ReadSec() const
{
	return (GetTime() - started_at);
}