// Fill out your copyright notice in the Description page of Project Settings.


#include "AdvPhysScene.h"

// Sets default values
AAdvPhysScene::AAdvPhysScene()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AAdvPhysScene::AddPhysObject(UStaticMeshComponent* Component)
{
	RecordData = {};
	Status = {};
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
	RecordData = {};
	Status = {};
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
	FMessageLog("AdvPhysScene").Info(
		FText::Format(
			FText::FromString("Start Recording, {0} objects, {1} frames, {2}s each."),
			PhysObjects.Num(),
			FrameCount,
			Interval
			)
		);
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
	if (RecordData.FrameCount <= 0)
	{
		FMessageLog("AdvPhysScene").Error(FText::FromString("Tried to play with no recorded data!"));
		return;
	}
	FMessageLog("AdvPhysScene").Info(
		FText::Format(
			FText::FromString("Start Playing, {0} objects, {1} frames, {2}s each."),
			PhysObjects.Num(),
			RecordData.FrameCount,
			RecordData.FrameInterval
			)
		);
	
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
	return Status.CurrentFrame >= RecordData.FrameCount;
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
		const FPhysEntry& Entry = RecordData.Entries[Status.CurrentFrame * NumOfObjects + ObjIndex];
		if (bUseNoPhysicsSetLocAndRot)
		{
			PhysObjects[ObjIndex].Comp->SetWorldLocationAndRotationNoPhysics(Entry.Location, Entry.Rotation);
		}
		else
		{
			PhysObjects[ObjIndex].Comp->SetWorldLocationAndRotation(Entry.Location, Entry.Rotation);
		}
	}
}

void AAdvPhysScene::AdvanceFrame()
{
	if (IsFrameCursorAtEnd())
	{
		FMessageLog("AdvPhysScene").Error(FText::FromString("Frame cursor out of bounds!"));
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
			FMessageLog("AdvPhysScene").Info(FText::FromString("Recording finished."));
			Status = {};
			StopSimulation();
		}
		break;
	case Playing:
		if (!ShouldRecordOrPlayFrame()) return;
		PlayFrame();
		AdvanceFrame();
		if (IsFrameCursorAtEnd())
		{
			FMessageLog("AdvPhysScene").Info(FText::FromString("Playing finished."));
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

