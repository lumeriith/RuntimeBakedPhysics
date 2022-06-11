// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../../../../../../../../Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.31.31103/INCLUDE/any"
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
	virtual std::any GetEvent();
	
	friend class AAdvPhysScene;
};
