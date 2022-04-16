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
	int RecordCursor;
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

	UPROPERTY(EditAnywhere)
	bool bEnableInterpolation = true;
	
	virtual void Tick(float DeltaTime) override;
protected:
	virtual void BeginPlay() override;
	
	inline bool IsRecordCursorAtEnd() const;
	inline float GetDuration() const;
	
	void TickRecording();
	void TickPlaying();
	
	void RecordFrame();
	void PlayFrame(float Time);
	
	inline bool ShouldRecordFrame() const;
	
	
	FStatus Status;
	TArray<FPhysObject> PhysObjects;
	FPhysRecordData RecordData;
};
