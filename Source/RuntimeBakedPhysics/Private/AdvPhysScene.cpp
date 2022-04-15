// Fill out your copyright notice in the Description page of Project Settings.


#include "AdvPhysScene.h"

// Sets default values
AAdvPhysScene::AAdvPhysScene()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

void AAdvPhysScene::AddPhysObject(UStaticMeshComponent* Component)
{
	PhysObject NewObj;
	NewObj.Comp = Component;
	NewObj.InitLoc = Component->GetComponentLocation();
	NewObj.InitRot = Component->GetComponentRotation();
	NewObj.ShouldSimulate = Component->IsSimulatingPhysics();
	Component->SetSimulatePhysics(false);
	physObjects.Add(NewObj);
}

void AAdvPhysScene::ClearPhysObjects()
{
	physObjects.Empty();
}

void AAdvPhysScene::ResetSimulation()
{
	for (const auto& OBJ : physObjects)
	{
		OBJ.Comp->SetWorldLocationAndRotation(OBJ.InitLoc, OBJ.InitRot);
	}
}

void AAdvPhysScene::StartSimulation()
{
	for (const auto& OBJ : physObjects)
	{
		OBJ.Comp->SetSimulatePhysics(OBJ.ShouldSimulate);
	}
}

void AAdvPhysScene::StopSimulation()
{
	for (const auto& OBJ : physObjects)
	{
		OBJ.Comp->SetSimulatePhysics(false);
	}
}

// Called when the game starts or when spawned
void AAdvPhysScene::BeginPlay()
{
	Super::BeginPlay();

}

// Called every frame
void AAdvPhysScene::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


