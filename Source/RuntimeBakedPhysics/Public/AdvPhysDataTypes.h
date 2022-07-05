#pragma once
#include "AdvPhysDataTypes.generated.h"

class AAdvPhysEventBase;

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
	AAdvPhysEventBase* EventActor;
	std::shared_ptr<FPhysEventNode> Next;
};

struct FPhysObjLocRot
{
	FVector Location;
	FRotator Rotation;
};

struct FPhysObjSODData
{
	FBox Bounds;
	uint32 StartHash;
	uint32 EndHash;
};

USTRUCT(BlueprintType)
struct FPhysRecordData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	bool Finished;

	UPROPERTY(BlueprintReadOnly)
	float Progress;

	UPROPERTY(BlueprintReadOnly)
	int FrameCount;

	UPROPERTY(BlueprintReadOnly)
	float FrameInterval;

	UPROPERTY(BlueprintReadOnly)
	bool bEnableSOD;

	UPROPERTY(BlueprintReadOnly)
	FVector HashWorldCenter;

	UPROPERTY(BlueprintReadOnly)
	float HashCellSize;
	
	TArray<FPhysObjLocRot> ObjLocRot;
	TArray<FPhysObjSODData> ObjSOD;
};