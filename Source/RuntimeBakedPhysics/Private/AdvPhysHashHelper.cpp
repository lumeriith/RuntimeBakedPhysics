#include "AdvPhysHashHelper.h"

void AdvPhysHashHelper::GetHash(const physx::PxBounds3 Bounds,
	const FVector WorldCenter, const float CellSize, uint32& Start, uint32& End)
{
	const double OffsetMetersFromCenter = CellSize * WORLD_CELL_LENGTH / 2.0f;

	const double CellWorldStartX = WorldCenter.X - OffsetMetersFromCenter;
	const double CellWorldStartY = WorldCenter.Y - OffsetMetersFromCenter;
	const double CellWorldStartZ = WorldCenter.Z - OffsetMetersFromCenter;

	const unsigned StartCellX = static_cast<unsigned>((Bounds.minimum.x - CellWorldStartX) / CellSize);
	const unsigned StartCellY = static_cast<unsigned>((Bounds.minimum.y - CellWorldStartY) / CellSize);
	const unsigned StartCellZ = static_cast<unsigned>((Bounds.minimum.z - CellWorldStartZ) / CellSize);

	const unsigned EndCellX = static_cast<unsigned>((Bounds.maximum.x - CellWorldStartX) / CellSize);
	const unsigned EndCellY = static_cast<unsigned>((Bounds.maximum.y - CellWorldStartY) / CellSize);
	const unsigned EndCellZ = static_cast<unsigned>((Bounds.maximum.z - CellWorldStartZ) / CellSize);

	Start = JoinToHash(StartCellX, StartCellY, StartCellZ);
	End = JoinToHash(EndCellX, EndCellY, EndCellZ);
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

void AdvPhysHashHelper::CubicSweepHash(const uint32 Start, const uint32 End, void(* Action)(uint32))
{
	unsigned MinX, MinY, MinZ, MaxX, MaxY, MaxZ;
	SplitFromHash(Start, MinX, MinY, MinZ);
	SplitFromHash(End, MaxX, MaxY, MaxZ);
	for (unsigned X = MinX; X <= MaxX; X++)
		for (unsigned Y = MinY; Y <= MaxY; Y++)
			for (unsigned Z = MinZ; X <= MaxZ; Z++)
				Action(JoinToHash(X, Y, Z));
}
