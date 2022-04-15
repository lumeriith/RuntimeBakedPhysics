// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
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
	int CurrentFrame;
	float StartTime;
};

struct FPhysObject
{
	UStaticMeshComponent* Comp;
	FVector InitLoc;
	FRotator InitRot;
	bool ShouldSimulate;
};

struct FPhysEntry
{
	FVector Location;
	FRotator Rotation;
};

struct FPhysRecordData
{
	int FrameCount;
	float FrameInterval;
	TArray<FPhysEntry> Entries;
};

UCLASS()
class RUNTIMEBAKEDPHYSICS_API AAdvPhysScene : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AAdvPhysScene();
	UFUNCTION(BlueprintCallable)
	void AddPhysObject(UStaticMeshComponent* Component);
	UFUNCTION(BlueprintCallable)
	void ClearPhysObjects();

	UFUNCTION(BlueprintCallable)
	void ResetObjects();
	UFUNCTION(BlueprintCallable)
	void StartSimulation();
	UFUNCTION(BlueprintCallable)
	void StopSimulation();

	UFUNCTION(BlueprintCallable)
	void Record(const float Interval, const int FrameCount);
	UFUNCTION(BlueprintCallable)
	void Play();
	UFUNCTION(BlueprintCallable)
	void Cancel();

	bool IsFrameCursorAtEnd() const;
	
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere)
	bool bUseNoPhysicsSetLocAndRot;

	
	
protected:
	virtual void BeginPlay() override;
	
	void RecordFrame();
	void PlayFrame();
	void AdvanceFrame();
	bool ShouldRecordOrPlayFrame() const;
	
	
	FStatus Status;
	TArray<FPhysObject> PhysObjects;
	FPhysRecordData RecordData;
};
