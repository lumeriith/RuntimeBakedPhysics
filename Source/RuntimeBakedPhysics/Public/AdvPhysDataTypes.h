#pragma once

struct FPhysObject
{
	UStaticMeshComponent* Comp;
	FVector InitLoc;
	FRotator InitRot;
	bool IsStatic;
};

struct FPhysFrame
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
	TArray<FPhysFrame> Frames;
};