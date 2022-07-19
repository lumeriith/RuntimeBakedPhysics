// Fill out your copyright notice in the Description page of Project Settings.


#include "AdvPhysSceneController.h"

// Sets default values
AAdvPhysSceneController::AAdvPhysSceneController()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AAdvPhysSceneController::BeginPlay()
{
	Super::BeginPlay();
	
}

void AAdvPhysSceneController::BeginRecordScene(PhysSimulator* Sim)
{
}

void AAdvPhysSceneController::RecordSceneTick(PhysSimulator* Sim, int FrameIndex)
{
}

void AAdvPhysSceneController::EndRecordScene(PhysSimulator* Sim)
{
}

void AAdvPhysSceneController::BeginPlayScene(AAdvPhysScene* Scene)
{
}

void AAdvPhysSceneController::DidStartSimulateOnDemand(AAdvPhysScene* Scene, int ObjIndex, int FrameIndex)
{
}

// Called every frame
void AAdvPhysSceneController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

