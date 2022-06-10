#pragma once

struct FPhysObject
{
	explicit FPhysObject(UStaticMeshComponent* Comp) :
		Comp(Comp),
		Location(Comp->GetComponentLocation()),
		Rotation(Comp->GetComponentRotation())
	{ }
	
	UStaticMeshComponent* Comp;
	FVector Location;
	FRotator Rotation;
};

enum EPhysEventType : uint8
{
	Explosive
};


struct FPhysEvent
{
	
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