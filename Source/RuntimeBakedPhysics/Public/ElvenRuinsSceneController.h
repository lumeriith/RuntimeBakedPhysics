// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AdvPhysSceneController.h"
#include "ElvenRuinsSceneController.generated.h"

/**
 * 
 */
UCLASS()
class RUNTIMEBAKEDPHYSICS_API AElvenRuinsSceneController : public AAdvPhysSceneController
{
	GENERATED_BODY()
public:
	virtual void BeginRecordScene(PhysSimulator* Sim) override;
};
