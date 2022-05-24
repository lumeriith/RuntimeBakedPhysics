// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AdvPhysDataTypes.h"
#include "AdvPhysParent.h"
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
	float StartTime;
	int Cursor;
};

UCLASS()
class RUNTIMEBAKEDPHYSICS_API AAdvPhysScene : public AActor
{
	GENERATED_BODY()

public:
	AAdvPhysScene();
	
	UFUNCTION(BlueprintCallable)
		void AddPhysObject(UStaticMeshComponent* Component);
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
	
	virtual void Tick(float DeltaTime) override;
protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	inline bool IsCursorAtEnd() const;
	inline float GetDuration() const;

	void DoRecordTick();
	void DoPlayTick();

	void PlayFrame(float Time);

	void ResetPhysObjectsPosition();

	PhysSimulator Simulator;
	FStatus Status;
	TArray<FPhysObject> PhysObjects;
	FPhysRecordData RecordData;
};
