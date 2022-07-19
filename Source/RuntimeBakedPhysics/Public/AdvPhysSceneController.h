// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PhysSimulator.h"
#include "GameFramework/Actor.h"
#include "AdvPhysSceneController.generated.h"

UCLASS()
class RUNTIMEBAKEDPHYSICS_API AAdvPhysSceneController : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AAdvPhysSceneController();
	
	virtual void BeginRecordScene(class PhysSimulator* Sim);
	virtual void RecordSceneTick(class PhysSimulator* Sim, int FrameIndex);
	virtual void EndRecordScene(class PhysSimulator* Sim);
	virtual void BeginPlayScene(class AAdvPhysScene* Scene);
	virtual void DidStartSimulateOnDemand(class AAdvPhysScene* Scene, int ObjIndex, int FrameIndex);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
