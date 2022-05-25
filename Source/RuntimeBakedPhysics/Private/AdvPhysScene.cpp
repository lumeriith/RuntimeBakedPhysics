// Fill out your copyright notice in the Description page of Project Settings.


#include "AdvPhysScene.h"

#include "AdvPhysParent.h"

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
	NewObj.IsStatic = !Component->IsSimulatingPhysics();
	Component->SetSimulatePhysics(false);
	PhysObjects.Add(NewObj);
	
	Simulator.AddToScene(Component);
}

void AAdvPhysScene::ClearPhysObjects()
{
	RecordData = {};
	Status = {};
	PhysObjects.Empty();
	Simulator.ClearScene();
}

void AAdvPhysScene::ResetPhysObjectsPosition()
{
	for (const auto& Obj : PhysObjects)
	{
		Obj.Comp->SetWorldLocationAndRotation(Obj.InitLoc, Obj.InitRot);
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
	RecordData = {};
	Simulator.StartRecord(&RecordData, Interval, FrameCount, GetWorld()->GetGravityZ());
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

	for (const auto& OBJ : PhysObjects)
	{
		OBJ.Comp->SetSimulatePhysics(false);
	}
	PlayFrame(0.0f);
}

void AAdvPhysScene::Cancel()
{
	ResetPhysObjectsPosition();
	if (Simulator.IsRecording())
	{
		Simulator.StopRecord();
		while (Simulator.IsRecording())
		{
			// Spin lock TODO
		}
	}
	Status = {};
}

bool AAdvPhysScene::IsCursorAtEnd() const
{
	return Status.Cursor >= RecordData.FrameCount;
}

float AAdvPhysScene::GetDuration() const
{
	return RecordData.FrameCount * RecordData.FrameInterval;
}

void AAdvPhysScene::PlayFrame(float Time)
{
	const float frame = Time / RecordData.FrameInterval;
	int StartFrame = FMath::FloorToInt(frame);
	int EndFrame = FMath::CeilToInt(frame);

	if (StartFrame >= RecordData.FrameCount)
		StartFrame = RecordData.FrameCount - 1;
	if (EndFrame >= RecordData.FrameCount)
		EndFrame = RecordData.FrameCount - 1;

	const size_t NumOfObjects = PhysObjects.Num();

	// Set location/rotation as-is
	if (!bEnableInterpolation || StartFrame == EndFrame)
	{
		for (int ObjIndex = 0; ObjIndex < NumOfObjects; ObjIndex++)
		{
			const FPhysObjLocRot& StartEntry = RecordData.ObjLocRot[StartFrame * NumOfObjects + ObjIndex];

			PhysObjects[ObjIndex].Comp->SetWorldLocationAndRotationNoPhysics(StartEntry.Location, StartEntry.Rotation);
		}
		return;
	}

	// Interpolate between two frames
	const float Value = frame - StartFrame;
	for (int ObjIndex = 0; ObjIndex < NumOfObjects; ObjIndex++)
	{
		const FPhysObjLocRot& StartEntry = RecordData.ObjLocRot[StartFrame * NumOfObjects + ObjIndex];
		const FPhysObjLocRot& EndEntry = RecordData.ObjLocRot[EndFrame * NumOfObjects + ObjIndex];

		const FVector Loc = StartEntry.Location * (1.0f - Value) + EndEntry.Location * Value;
		const FRotator Rot = FMath::Lerp(StartEntry.Rotation, EndEntry.Rotation, Value);

		PhysObjects[ObjIndex].Comp->SetWorldLocationAndRotationNoPhysics(Loc, Rot);
	}
}

// Called every frame
void AAdvPhysScene::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	switch (Status.Current)
	{
	case Idle: break;
	case Recording:
		DoRecordTick();
		break;
	case Playing:
		DoPlayTick();
		break;
	default:;
	}
}

void AAdvPhysScene::BeginPlay()
{
	Super::BeginPlay();
	Simulator.Initialize();
}

void AAdvPhysScene::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	Simulator.Cleanup();
}

void AAdvPhysScene::DoRecordTick()
{
	if (RecordData.Finished)
	{
		FMessageLog("AdvPhysScene").Info(FText::FromString("Recording finished."));
		Status = {};
	}
}

void AAdvPhysScene::DoPlayTick()
{
	const float currentTime = GetWorld()->GetTimeSeconds() - Status.StartTime;

	PlayFrame(currentTime);

	if (currentTime > GetDuration())
	{
		FMessageLog("AdvPhysScene").Info(FText::FromString("Playing finished."));
		Status = {};
	}
}

