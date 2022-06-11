#pragma once
#include "../../../../../../../../Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.31.31103/INCLUDE/any"
#include "AdvPhysDataTypes.generated.h"

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

struct FPhysEventNode
{
	std::any Event;
	std::shared_ptr<FPhysEventNode> Next;
};

USTRUCT()
struct FPhysEvent_Explosion
{
	GENERATED_BODY()
	
	FVector Position;
	float Impulse;
	float FallOffMinDistance;
	float FallOffMaxDistance;
};

union FPhysEvent
{
	FPhysEvent_Explosion Explosion;
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