#include "AdvPhysHashHelper.h"

void AdvPhysHashHelper::GetHash(const physx::PxBounds3 Bounds,
	const FVector WorldCenter, const float CellSize, uint32& Start, uint32& End)
{
	Start = GetHash(Bounds.minimum, WorldCenter, CellSize);
	End = GetHash(Bounds.maximum, WorldCenter, CellSize);
}

void AdvPhysHashHelper::GetHash(FBox Bounds, FVector WorldCenter, float CellSize, uint32& Start, uint32& End)
{
	Start = GetHash(Bounds.Min, WorldCenter, CellSize);
	End = GetHash(Bounds.Max, WorldCenter, CellSize);
}

uint32 AdvPhysHashHelper::GetHash(const physx::PxVec3 Point, const FVector WorldCenter, const float CellSize)
{
	const double OffsetMetersFromCenter = CellSize * WORLD_CELL_LENGTH / 2.0f;

	const double CellWorldStartX = WorldCenter.X - OffsetMetersFromCenter;
	const double CellWorldStartY = WorldCenter.Y - OffsetMetersFromCenter;
	const double CellWorldStartZ = WorldCenter.Z - OffsetMetersFromCenter;

	const unsigned StartCellX = static_cast<unsigned>((Point.x - CellWorldStartX) / CellSize);
	const unsigned StartCellY = static_cast<unsigned>((Point.y - CellWorldStartY) / CellSize);
	const unsigned StartCellZ = static_cast<unsigned>((Point.z - CellWorldStartZ) / CellSize);

	return JoinToHash(StartCellX, StartCellY, StartCellZ);
}

uint32 AdvPhysHashHelper::GetHash(FVector Point, FVector WorldCenter, float CellSize)
{
	const double OffsetMetersFromCenter = CellSize * WORLD_CELL_LENGTH / 2.0f;

	const double CellWorldStartX = WorldCenter.X - OffsetMetersFromCenter;
	const double CellWorldStartY = WorldCenter.Y - OffsetMetersFromCenter;
	const double CellWorldStartZ = WorldCenter.Z - OffsetMetersFromCenter;

	const unsigned StartCellX = static_cast<unsigned>((Point.X - CellWorldStartX) / CellSize);
	const unsigned StartCellY = static_cast<unsigned>((Point.Y - CellWorldStartY) / CellSize);
	const unsigned StartCellZ = static_cast<unsigned>((Point.Z - CellWorldStartZ) / CellSize);

	return JoinToHash(StartCellX, StartCellY, StartCellZ);
}

void AdvPhysHashHelper::SplitFromHash(const uint32 Hash, unsigned& X, unsigned& Y, unsigned& Z)
{
	X = Hash / (WORLD_CELL_LENGTH * WORLD_CELL_LENGTH);
	Y = (Hash - X * WORLD_CELL_LENGTH * WORLD_CELL_LENGTH) / WORLD_CELL_LENGTH;
	Z = (Hash - X * WORLD_CELL_LENGTH * WORLD_CELL_LENGTH - Y * WORLD_CELL_LENGTH);
}

int32 AdvPhysHashHelper::JoinToHash(const unsigned X, const unsigned Y, const unsigned Z)
{
	return X * WORLD_CELL_LENGTH * WORLD_CELL_LENGTH + Y * WORLD_CELL_LENGTH + Z;
}

void AdvPhysHashHelper::CubicSweepHash(uint32 Start, uint32 End, std::function<void(uint32)> Action)
{
	unsigned MinX, MinY, MinZ, MaxX, MaxY, MaxZ;
	SplitFromHash(Start, MinX, MinY, MinZ);
	SplitFromHash(End, MaxX, MaxY, MaxZ);
	for (unsigned X = MinX; X <= MaxX; X++)
		for (unsigned Y = MinY; Y <= MaxY; Y++)
			for (unsigned Z = MinZ; Z <= MaxZ; Z++)
				Action(JoinToHash(X, Y, Z));
}
