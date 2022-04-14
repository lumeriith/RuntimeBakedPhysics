// Fill out your copyright notice in the Description page of Project Settings.


#include "APhysicsScene.h"

// Sets default values
AAPhysicsScene::AAPhysicsScene()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

void AAPhysicsScene::AddPhysObject(UStaticMeshComponent* Component)
{
	PhysObject NewObj;
	NewObj.Comp = Component;
	NewObj.InitLoc = Component->GetComponentLocation();
	NewObj.InitRot = Component->GetComponentRotation();
	NewObj.ShouldSimulate = Component->IsSimulatingPhysics();
	Component->SetSimulatePhysics(false);
	physObjects.Add(NewObj);
}

void AAPhysicsScene::ClearPhysObjects()
{
	physObjects.Empty();
}

void AAPhysicsScene::ResetSimulation()
{
	for (const auto& OBJ : physObjects)
	{
		OBJ.Comp->SetWorldLocationAndRotation(OBJ.InitLoc, OBJ.InitRot);
	}
}

void AAPhysicsScene::StartSimulation()
{
	for (const auto& OBJ : physObjects)
	{
		OBJ.Comp->SetSimulatePhysics(OBJ.ShouldSimulate);
	}
}

void AAPhysicsScene::StopSimulation()
{
	for (const auto& OBJ : physObjects)
	{
		OBJ.Comp->SetSimulatePhysics(false);
	}
}

// Called when the game starts or when spawned
void AAPhysicsScene::BeginPlay()
{
	Super::BeginPlay();

}

// Called every frame
void AAPhysicsScene::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

