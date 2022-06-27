// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AdvPhysDataTypes.h"
#include "AdvPhysEventBase.h"
#include "PhysSimulator.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/Actor.h"
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
	float PlayLastFrameTime;
	TArray<AAdvPhysEventBase*> PlayEventActors;
};

DECLARE_MULTICAST_DELEGATE(FRecordFinishedDeleagte)

UCLASS()
class RUNTIMEBAKEDPHYSICS_API AAdvPhysScene : public AActor
{
	GENERATED_BODY()
	
public:
	AAdvPhysScene();
	
	UFUNCTION(BlueprintCallable)
		void AddDynamicObject(UStaticMeshComponent* Component);
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
		EAction GetAction() const;

	UFUNCTION(BlueprintCallable)
		float GetRecordProgress() const;

	UFUNCTION(BlueprintCallable)
		float GetPlayFramesPerSecond() const;

	UFUNCTION(BlueprintCallable)
		int GetRecordDataFrameCount() const;

	UFUNCTION(BlueprintCallable)
		float GetRecordDataFrameInterval() const;

	UFUNCTION(BlueprintCallable)
		bool GetRecordDataFinished() const;
	
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
	bool bFreezeDynamicObjectOnAdd = true;

	UPROPERTY(EditAnywhere)
	TArray<AAdvPhysEventBase*> EventActors;

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

	void AddTaggedObjects();
	void ResetPhysObjectsPosition();

	void CopyObjectsToSimulator();

	PhysSimulator Simulator;
	FStatus Status;
	
	TArray<FPhysObject> DynamicObjEntries;
	TArray<FPhysObject> StaticObjEntries;
	
	FPhysRecordData RecordData;
	double RecordStartTime;
};
