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
	
	TArray<FPhysObjLocRot> ObjLocRot;
};