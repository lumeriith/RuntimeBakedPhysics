#pragma once

struct FPhysObject
{
	UStaticMeshComponent* Comp;
	FVector InitLoc;
	FRotator InitRot;
	bool IsStatic;
};

struct FPhysObjLocRot
{
	FVector Location;
	FRotator Rotation;
};

struct FPhysRecordData
{
	bool Finished;
	float Progress;
	int FrameCount;
	float FrameInterval;
	TArray<FPhysObjLocRot> ObjLocRot;
};