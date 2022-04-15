// Fill out your copyright notice in the Description page of Project Settings.


#include "AdvPhysScene.h"

// Sets default values
AAdvPhysScene::AAdvPhysScene()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AAdvPhysScene::AddPhysObject(UStaticMeshComponent* Component)
{
	FPhysObject NewObj;
	NewObj.Comp = Component;
	NewObj.InitLoc = Component->GetComponentLocation();
	NewObj.InitRot = Component->GetComponentRotation();
	NewObj.ShouldSimulate = Component->IsSimulatingPhysics();
	Component->SetSimulatePhysics(false);
	PhysObjects.Add(NewObj);
}

void AAdvPhysScene::ClearPhysObjects()
{
	PhysObjects.Empty();
}

void AAdvPhysScene::ResetObjects()
{
	for (const auto& OBJ : PhysObjects)
	{
		OBJ.Comp->SetWorldLocationAndRotation(OBJ.InitLoc, OBJ.InitRot);
	}
}

void AAdvPhysScene::StartSimulation()
{
	for (const auto& OBJ : PhysObjects)
	{
		OBJ.Comp->SetSimulatePhysics(OBJ.ShouldSimulate);
	}
}

void AAdvPhysScene::StopSimulation()
{
	for (const auto& OBJ : PhysObjects)
	{
		OBJ.Comp->SetSimulatePhysics(false);
	}
}

void AAdvPhysScene::Record(const float Interval, const int FrameCount)
{
	Cancel();
	Status.Current = Recording;
	Status.StartTime = GetWorld()->GetTimeSeconds();
	
	RecordData.FrameCount = FrameCount;
	RecordData.FrameInterval = Interval;

	RecordData.Entries.Init(FPhysEntry(), FrameCount * PhysObjects.Num());
	
	StartSimulation();
	RecordFrame();
	AdvanceFrame();
}

void AAdvPhysScene::Play()
{
	Cancel();
	Status.Current = Playing;
	Status.StartTime = GetWorld()->GetTimeSeconds();

	StopSimulation();
	PlayFrame();
	AdvanceFrame();
}

void AAdvPhysScene::Cancel()
{
	ResetObjects();
	Status = {};
}

bool AAdvPhysScene::IsFrameCursorAtEnd() const
{
	return Status.CurrentFrame >= RecordData.FrameCount * PhysObjects.Num();
}

void AAdvPhysScene::RecordFrame()
{
	if (IsFrameCursorAtEnd())
	{
		FMessageLog("AdvPhysScene").Error(FText::FromString("Current frame out of bounds!"));
		return;
	}

	const size_t NumOfObjects = PhysObjects.Num();
	for (int ObjIndex = 0; ObjIndex < NumOfObjects; ObjIndex++)
	{
		const FVector Loc = PhysObjects[ObjIndex].Comp->GetComponentLocation();
		const FRotator Rot = PhysObjects[ObjIndex].Comp->GetComponentRotation();

		FPhysEntry& Entry = RecordData.Entries[Status.CurrentFrame * NumOfObjects + ObjIndex];
		Entry.Location = Loc;
		Entry.Rotation = Rot;
	}
}

void AAdvPhysScene::PlayFrame()
{
	if (IsFrameCursorAtEnd())
	{
		FMessageLog("AdvPhysScene").Error(FText::FromString("Current frame out of bounds!"));
		return;
	}

	const size_t NumOfObjects = PhysObjects.Num();
	for (int ObjIndex = 0; ObjIndex < NumOfObjects; ObjIndex++)
	{
		const FPhysEntry& Entry = RecordData.Entries[ObjIndex];
		PhysObjects[Status.CurrentFrame * NumOfObjects + ObjIndex].Comp->SetWorldLocationAndRotation(Entry.Location, Entry.Rotation);
	}
}

void AAdvPhysScene::AdvanceFrame()
{
	if (IsFrameCursorAtEnd())
	{
		FMessageLog("AdvPhysScene").Error(FText::FromString("Record cursor out of bounds!"));
		return;
	}
	Status.CurrentFrame++;
}

bool AAdvPhysScene::ShouldRecordOrPlayFrame() const
{
	if (IsFrameCursorAtEnd()) return false;

	const float DesiredGameTime = Status.StartTime + Status.CurrentFrame * RecordData.FrameInterval;
	return GetWorld()->GetTimeSeconds() >= DesiredGameTime;
}

// Called every frame
void AAdvPhysScene::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	switch(Status.Current)
	{
	case Idle: break;
	case Recording:
		if (!ShouldRecordOrPlayFrame()) return;
		RecordFrame();
		AdvanceFrame();
		if (IsFrameCursorAtEnd())
		{
			Status = {};
		}
		break;
	case Playing:
		if (!ShouldRecordOrPlayFrame()) return;
		PlayFrame();
		AdvanceFrame();
		if (IsFrameCursorAtEnd())
		{
			Status = {};
		}
		break;
	default: ;
	}
}

// Called when the game starts or when spawned
void AAdvPhysScene::BeginPlay()
{
	Super::BeginPlay();

}

