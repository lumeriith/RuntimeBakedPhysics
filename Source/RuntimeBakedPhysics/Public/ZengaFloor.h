// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZengaFloor.generated.h"

UCLASS()
class RUNTIMEBAKEDPHYSICS_API AZengaFloor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AZengaFloor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere)
		UMaterial* sleepMaterial;

	UPROPERTY(EditAnywhere)
		UMaterial* awakeMaterial;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
