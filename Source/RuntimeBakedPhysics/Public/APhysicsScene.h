// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "APhysicsScene.generated.h"

struct PhysObject
{
	UStaticMeshComponent* Comp;
	FVector InitLoc;
	FRotator InitRot;
	bool ShouldSimulate;
};

UCLASS()
class RUNTIMEBAKEDPHYSICS_API AAPhysicsScene : public AActor
{
	GENERATED_BODY()

public:
	AAPhysicsScene();

	UFUNCTION(BlueprintCallable)
		void AddPhysObject(UStaticMeshComponent* Component);
	UFUNCTION(BlueprintCallable)
		void ClearPhysObjects();

	UFUNCTION(BlueprintCallable)
		void ResetSimulation();
	UFUNCTION(BlueprintCallable)
		void StartSimulation();
	UFUNCTION(BlueprintCallable)
		void StopSimulation();

protected:
	virtual void BeginPlay() override;

	TArray<PhysObject> physObjects;

public:
	virtual void Tick(float DeltaTime) override;
};
