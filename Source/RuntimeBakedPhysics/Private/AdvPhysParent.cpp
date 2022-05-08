// Fill out your copyright notice in the Description page of Project Settings.


#include "AdvPhysParent.h"

AAdvPhysParent::AAdvPhysParent(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
	Scene = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(this, TEXT("Scene"));
	Scene->SetMobility(EComponentMobility::Movable);
	SetRootComponent(Scene);
}
