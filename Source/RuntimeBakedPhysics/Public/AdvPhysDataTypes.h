#pragma once
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "AdvPhysDataTypes.generated.h"

class AAdvPhysEventBase;

UENUM()
enum EShapeType
{
	Simple,
	Aggregate,
	TriMesh
};

struct FPhysObject
{
	explicit FPhysObject(UStaticMeshComponent* Comp) :
		Comp(Comp),
		CollisionProfile(Comp->GetCollisionProfileName()),
		Location(Comp->GetComponentLocation()),
		Rotation(Comp->GetComponentRotation())
	{ }
	
	UStaticMeshComponent* Comp;
	FName CollisionProfile;
	FVector Location;
	FRotator Rotation;
};

struct FPhysGeomCollection
{
	explicit FPhysGeomCollection(UGeometryCollectionComponent* Comp) :
		Comp(Comp)
	{ }

	UGeometryCollectionComponent* Comp;
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