// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AdvPhysSceneController.h"
#include "WildWestSceneController.generated.h"

/**
 * 
 */
UCLASS()
class RUNTIMEBAKEDPHYSICS_API AWildWestSceneController : public AAdvPhysSceneController
{
	GENERATED_BODY()

public:
	virtual void BeginRecordScene(PhysSimulator* Sim) override;
	virtual void RecordSceneTick(PhysSimulator* Sim, int FrameIndex) override;

	UPROPERTY(EditAnywhere)
	int StartFrame;

	UPROPERTY(EditAnywhere)
	int EndFrame;

private:
	std::vector<int> Indices;
};
