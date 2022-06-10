// Fill out your copyright notice in the Description page of Project Settings.


#include "AdvPhysScene.h"

#include "AdvPhysParent.h"

// Sets default values
AAdvPhysScene::AAdvPhysScene()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AAdvPhysScene::AddDynamicObject(UStaticMeshComponent* Component)
{
	if (!Component)
	{
		FMessageLog("AdvPhysScene").Error(FText::FromString("AddDynamicObject invalid argument"));
		return;
	}
	
	RecordData = FPhysRecordData();
	Status = FStatus();
	
	DynamicUEObjects.Add(FPhysObject(Component));
}

void AAdvPhysScene::AddStaticObject(UStaticMeshComponent* Component)
{
	if (!Component)
	{
		FMessageLog("AdvPhysScene").Error(FText::FromString("AddStaticObject invalid argument"));
		return;
	}
	
	RecordData = FPhysRecordData();
	Status = FStatus();
	
	StaticUEObjects.Add(FPhysObject(Component));
}

void AAdvPhysScene::ClearPhysObjects()
{
	RecordData = FPhysRecordData();
	Status = FStatus();
	DynamicUEObjects.Empty();
	StaticUEObjects.Empty();
	Simulator.ClearScene();
}

void AAdvPhysScene::ResetPhysObjectsPosition()
{
	for (const auto& Obj : DynamicUEObjects)
	{
		Obj.Comp->SetWorldLocationAndRotation(Obj.Location, Obj.Rotation);
	}
}

void AAdvPhysScene::CopyObjectsToSimulator()
{
	const double StartSeconds = FPlatformTime::Seconds();
	for (const auto& Obj : DynamicUEObjects)
	{
		Simulator.AddDynamicBody(Obj.Comp);
	}

	for (const auto& Obj : StaticUEObjects)
	{
		Simulator.AddStaticBody(Obj.Comp);
	}
	const double Now = FPlatformTime::Seconds();

	FMessageLog("AdvPhysScene").Info(
	FText::Format(
		FText::FromString("CopyObjectsToSimulator took {0}ms, {1} dynamic objects, {2} static objects."),
		(Now - StartSeconds) * 1000,
		DynamicUEObjects.Num(),
		StaticUEObjects.Num()
	));
}

void AAdvPhysScene::Record(const float Interval, const int FrameCount)
{
	FMessageLog("AdvPhysScene").Info(
		FText::Format(
			FText::FromString("Start Recording, {0} objects, {1} frames, {2}s each."),
			DynamicUEObjects.Num(),
			FrameCount,
			Interval
		)
	);
	
	Cancel();
	Status.Current = Recording;
	RecordData = {};
	CopyObjectsToSimulator();
	Simulator.ReserveEvents(FrameCount);
	Simulator.AddEvents(EventPairs);
	Simulator.StartRecord(&RecordData, Interval, FrameCount, GetWorld()->GetGravityZ());
	RecordStartTime = FPlatformTime::Seconds();
}

void AAdvPhysScene::Play()
{
	if (RecordData.FrameCount <= 0)
	{
		FMessageLog("AdvPhysScene").Error(FText::FromString("Tried to play with no recorded data."));
		return;
	}
	FMessageLog("AdvPhysScene").Info(
		FText::Format(
			FText::FromString("Start Playing, {0} objects, {1} frames, {2}s each."),
			DynamicUEObjects.Num(),
			RecordData.FrameCount,
			RecordData.FrameInterval
		)
	);

	Cancel();
	Status.Current = Playing;
	Status.PlayStartTime = GetWorld()->GetTimeSeconds();

	for (const auto& Obj : DynamicUEObjects)
	{
		Obj.Comp->SetSimulatePhysics(false);
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

	const size_t NumOfObjects = DynamicUEObjects.Num();

	// Set location/rotation as-is
	if (!bEnableInterpolation || StartFrame == EndFrame)
	{
		for (int ObjIndex = 0; ObjIndex < NumOfObjects; ObjIndex++)
		{
			const FPhysObjLocRot& StartEntry = RecordData.ObjLocRot[StartFrame * NumOfObjects + ObjIndex];

			DynamicUEObjects[ObjIndex].Comp->SetWorldLocationAndRotationNoPhysics(StartEntry.Location, StartEntry.Rotation);
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

		DynamicUEObjects[ObjIndex].Comp->SetWorldLocationAndRotationNoPhysics(Loc, Rot);
	}
}

void AAdvPhysScene::AddEvent(int Frame, std::any Event)
{
	EventPairs.Add(std::make_tuple(Frame, Event));
}

void AAdvPhysScene::ClearEvents()
{
	EventPairs.Empty();
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
		const double Now = FPlatformTime::Seconds();
		FMessageLog("AdvPhysScene").Info(FText::Format(
			FText::FromString("Recording finished, took {0} seconds."),
			Now - RecordStartTime
			));
		Status = {};
		Simulator.ClearScene();
		Simulator.FreeEvents();
	}
}

void AAdvPhysScene::DoPlayTick()
{
	const float Now = GetWorld()->GetTimeSeconds();
	if (PlayFramesPerSecond > 0.01f && Now - Status.PlayLastFrameTime < 1.0f / PlayFramesPerSecond)
	{
		return; // Skip frame
	}
	
	const float CurrentTime = Now - Status.PlayStartTime;
	PlayFrame(CurrentTime);
	Status.PlayLastFrameTime = Now;
	if (CurrentTime > GetDuration())
	{
		FMessageLog("AdvPhysScene").Info(FText::FromString("Playing finished."));
		Status = {};
	}
}

