// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AdvPhysSceneController.h"
#include "BouncyBallsSceneController.generated.h"

/**
 * 
 */
UCLASS()
class RUNTIMEBAKEDPHYSICS_API ABouncyBallsSceneController : public AAdvPhysSceneController
{
	GENERATED_BODY()

public:
	virtual void BeginRecordScene(PhysSimulator* Sim) override;
	virtual void BeginPlayScene(AAdvPhysScene* Scene) override;
	virtual void DidStartSimulateOnDemand(AAdvPhysScene* Scene, int ObjIndex, int FrameIndex) override;

	UPROPERTY(EditAnywhere)
	float Magnitude = 100;

	UPROPERTY(EditAnywhere)
	bool bVisualizeSoD = true;

	UPROPERTY(EditAnywhere)
	UMaterialInterface* ReplacedMaterial;

private:
	std::vector<UMaterialInterface*> Materials;
};
