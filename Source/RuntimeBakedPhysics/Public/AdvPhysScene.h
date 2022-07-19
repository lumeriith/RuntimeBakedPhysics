// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AdvPhysDataTypes.h"
#include "AdvPhysEventBase.h"
#include "PhysSimulator.h"
#include "GameFramework/Actor.h"
#include <hash_map>

#include "AdvPhysSceneController.h"
#include "AdvPhysScene.generated.h"


UENUM(BlueprintType)
enum EAction
{
	Idle = 0,
	Recording,
	Playing,
	PlayingRealtimeSimulation
};

struct FStatus
{
	EAction Current;
	float PlayStartTime;
	float LastPlayFrameTime;
	float LastSODCheckTime;
	TArray<AAdvPhysEventBase*> PlayEventActors;
	TArray<bool> SODActivationState;
	TArray<USceneComponent*> AddedActivators;
	std::hash_map<uint32, std::list<int>> SODMap;
};

DECLARE_MULTICAST_DELEGATE(FRecordFinishedDeleagte)

UCLASS()
class RUNTIMEBAKEDPHYSICS_API AAdvPhysScene : public AActor
{
	GENERATED_BODY()
	
public:
	AAdvPhysScene();
	
	UFUNCTION(BlueprintCallable)
		void AddDynamicObj(UStaticMeshComponent* Component);
	UFUNCTION(BlueprintCallable)
		void AddStaticObject(UStaticMeshComponent* Component);
	
	UFUNCTION(BlueprintCallable)
		void ClearPhysObjects();
	
	void Record(const float Interval, const int FrameCount);
	
	UFUNCTION(BlueprintCallable)
		void Play();
	UFUNCTION(BlueprintCallable)
		void PlayRealtimeSimulation();
	UFUNCTION(BlueprintCallable)
		void Cancel();
	
	UFUNCTION(BlueprintCallable)
		void FreezeDynamicObjects();
	UFUNCTION(BlueprintCallable)
		void UnfreezeDynamicObjects();

	UFUNCTION(BlueprintCallable)
		void SetPlayFramesPerSecond(float FPS);
	
	UFUNCTION(BlueprintCallable)
		void ConfigureSimulateOnDemand(const bool bEnabled, const float HashCellSize);
	
	UFUNCTION(BlueprintCallable)
		EAction GetAction() const;

	UFUNCTION(BlueprintCallable)
		float GetRecordProgress() const;

	UFUNCTION(BlueprintCallable)
		int GetNumOfActivators() const;
	
	UFUNCTION(BlueprintCallable)
		float GetPlayFramesPerSecond() const;

	UFUNCTION(BlueprintCallable)
		int GetRecordDataFrameCount() const;

	UFUNCTION(BlueprintCallable)
		float GetRecordDataFrameInterval() const;

	UFUNCTION(BlueprintCallable)
		bool GetRecordDataFinished() const;

	UFUNCTION(BlueprintCallable)
		bool GetEnableSOD() const;

	UFUNCTION(BlueprintCallable)
		void SetEnableSOD(bool bEnable);

	UFUNCTION(BlueprintCallable)
		void SetNaiveSODCheck(bool bNaive);

	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere)
	float PlayFramesPerSecond = -1;
	
	UPROPERTY(EditAnywhere)
	bool bEnableInterpolation = true;
	
	UPROPERTY(EditAnywhere)
	bool bAddTaggedObjectsOnBeginPlay = false;

	UPROPERTY(EditAnywhere)
	FName DynamicTag = FName("AdvPhysDynamic");

	UPROPERTY(EditAnywhere)
	FName StaticTag = FName("AdvPhysStatic");
	
	UPROPERTY(EditAnywhere)
	FName ActivatorTag = FName("AdvPhysActivator");
	
	UPROPERTY(EditAnywhere)
	bool bFreezeDynamicObjectOnAdd = true;

	UPROPERTY(EditAnywhere)
	TArray<AAdvPhysEventBase*> EventActors;
	
	UPROPERTY(EditAnywhere)
	AAdvPhysSceneController* Controller;

	UPROPERTY(EditAnywhere)
	bool bEnableSOD = false;
	
	UPROPERTY(EditAnywhere)
	float SODHashCellSize = 100.0f;

	UPROPERTY(EditAnywhere)
	float SODCheckFramesPerSecond = -1;

	UPROPERTY(EditAnywhere)
	double SODOriginalActivatorBoundExpansion = 50;

	UPROPERTY(EditAnywhere)
	double SODAddedActivatorBoundExpansion = 0;
	

	UPROPERTY(EditAnywhere)
	bool bUseNaiveSODCheck = false;
	
	UPROPERTY(EditAnywhere)
	bool bEnableSODChainReaction = false;

	UPROPERTY(EditAnywhere)
	bool bDrawSODObjectBoundsOnPlay = false;

	UPROPERTY(EditAnywhere)
	bool bDrawSODHashCubesOnPlay = false;
	
	UPROPERTY(EditAnywhere)
	bool bDrawSODActivatedObjectsOnPlay = false;

	UPROPERTY(EditAnywhere)
	bool bUseSimpleGeometryForDynamicObj = false;
	
	UPROPERTY(EditAnywhere)
	TEnumAsByte<EShapeType> StaticObjShapeType = TriMesh;

	TArray<FPhysObject> DynamicObjEntries;
	TArray<FPhysObject> StaticObjEntries;
	
	FRecordFinishedDeleagte RecordFinished;

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	inline float GetDuration() const;

	void DoRecordTick();
	void DoPlayTick();
	void DoPlayRealtimeSimulationTick();

	void PlayFrame(float Time);
	void HandleEventsInFrame(float Time, bool ApplyEventsToRealWorld);

	void CheckSODAtTime(float Time);
	bool CheckActivatorIntersect(const USceneComponent* Activator, const FPhysObjSODData& SODData, const bool bIsOriginal) const;
	
	void RebuildSODMap(int FrameIndex);
	void CheckFromSODMap(const USceneComponent* Activator, const int FrameIndex, const bool bIsOriginal);
	void SimulateObjectOnDemand(int ObjIndex, int FrameIndex);

	void AddTaggedObjects();
	void ResetPhysObjectsPosition();

	void CopyObjectsToSimulator();

	void DrawSODObjectBounds();
	void DrawSODHashCubes();
	void DrawSODActivatedObjects();
	

	PhysSimulator Simulator;
	FStatus Status;

	UPROPERTY()
	TArray<USceneComponent*> OriginalActivators;
	
	FPhysRecordData RecordData;
	double RecordStartTime;
};
