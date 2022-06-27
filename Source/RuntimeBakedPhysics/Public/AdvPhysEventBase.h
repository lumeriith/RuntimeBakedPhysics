// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <PxRigidDynamic.h>

#include "CoreMinimal.h"
#include "AdvPhysDataTypes.h"
#include "GameFramework/Actor.h"
#include "AdvPhysEventBase.generated.h"
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlay);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTrigger);

UCLASS()
class RUNTIMEBAKEDPHYSICS_API AAdvPhysEventBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AAdvPhysEventBase();
	
	virtual bool ShouldTickIfViewportsOnly() const override;
	virtual void Tick(float DeltaSeconds) override;
	
	UPROPERTY(BlueprintAssignable)
	FOnPlay OnPlay;
	UPROPERTY(BlueprintAssignable)
	FOnTrigger OnTrigger;
	
	UPROPERTY(EditAnywhere)
	float Time;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	void BroadcastOnPlay();
	void BroadcastOnTrigger();

	virtual void DoEventPhysX(std::vector<physx::PxRigidDynamic*>& Bodies);
	virtual void DoEventUE(TArray<FPhysObject>& Dynamics);
	
	friend class AAdvPhysScene;
	friend class PhysSimulator;
};
