// Fill out your copyright notice in the Description page of Project Settings.


#include "AdvPhysEventBase.h"
#include "Particles/ParticleSystemComponent.h"

// Sets default values
AAdvPhysEventBase::AAdvPhysEventBase()
{
	SetRootComponent(CreateDefaultSubobject<USceneComponent>("Scene Root Component"));
	PrimaryActorTick.bCanEverTick = true;
}

bool AAdvPhysEventBase::ShouldTickIfViewportsOnly() const
{
	return Super::ShouldTickIfViewportsOnly();
}

// Called when the game starts or when spawned
void AAdvPhysEventBase::BeginPlay()
{
	Super::BeginPlay();
	
}

void AAdvPhysEventBase::BroadcastOnPlay()
{
	OnPlay.Broadcast();
}

void AAdvPhysEventBase::BroadcastOnTrigger()
{
	OnTrigger.Broadcast();
}

// Called every frame
void AAdvPhysEventBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

std::any AAdvPhysEventBase::GetEvent()
{
	return nullptr;
}