// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AdvPhysParent.generated.h"

UCLASS()
class RUNTIMEBAKEDPHYSICS_API AAdvPhysParent : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AAdvPhysParent(const FObjectInitializer& ObjectInitializer);

	UPROPERTY()
	class USceneComponent *Scene;
};
