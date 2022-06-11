// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AdvPhysDataTypes.h"
#include "AdvPhysEventBase.h"
#include "PhysSimulator.h"
#include "GameFramework/Actor.h"
#include "AdvPhysScene.generated.h"

enum EAction : uint8
{
	Idle = 0,
	Recording,
	Playing
};

struct FStatus
{
	EAction Current;
	float PlayStartTime;
	float PlayLastFrameTime;
	TArray<AAdvPhysEventBase*> PlayEventActors;
};

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
	
	UFUNCTION(BlueprintCallable)
		void Record(const float Interval, const int FrameCount);
	UFUNCTION(BlueprintCallable)
		void Play();
	UFUNCTION(BlueprintCallable)
		void Cancel();

	UPROPERTY(EditAnywhere)
		bool bEnableInterpolation = true;

	UPROPERTY(EditAnywhere)
		TSubclassOf<AActor> TestActor;

	void AddEvent(float Time, std::any Event);

	void ClearEvents();
	
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere)
		float PlayFramesPerSecond = -1;

	UPROPERTY(EditAnywhere)
		TArray<AAdvPhysEventBase*> EventActors;

	
protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	inline float GetDuration() const;

	void DoRecordTick();
	void DoPlayTick();

	void PlayFrame(float Time);
	void BroadcastEventsToActorsFrame(float Time);

	void ResetPhysObjectsPosition();

	void CopyObjectsToSimulator();

	PhysSimulator Simulator;
	FStatus Status;
	
	TArray<FPhysObject> DynamicUEObjects;
	TArray<FPhysObject> StaticUEObjects;

	TArray<std::tuple<float, std::any>> EventPairs;
	
	FPhysRecordData RecordData;
	double RecordStartTime;
};
