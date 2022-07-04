#pragma once
#include <PxBounds3.h>

#define WORLD_CELL_LENGTH 1625

class AdvPhysHashHelper
{
public:
	static void GetHash(physx::PxBounds3 Bounds, FVector WorldCenter, float CellSize, uint32& Start, uint32& End);
	static void SplitFromHash(uint32 Hash, unsigned& X, unsigned& Y, unsigned& Z);
	static int32 JoinToHash(unsigned X, unsigned Y, unsigned Z);
	static void CubicSweepHash(uint32 Start, uint32 End, void (*Action)(uint32));	
private:
	AdvPhysHashHelper() {}
};