// Fill out your copyright notice in the Description page of Project Settings.


#include "AdvPhysScene.h"

#include "Kismet/GameplayStatics.h"

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
	
	DynamicObjEntries.Add(FPhysObject(Component));

	if (bFreezeDynamicObjectOnAdd)
	{
		Component->SetSimulatePhysics(false);
	}
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
	
	StaticObjEntries.Add(FPhysObject(Component));
}

void AAdvPhysScene::ClearPhysObjects()
{
	RecordData = FPhysRecordData();
	Status = FStatus();
	DynamicObjEntries.Empty();
	StaticObjEntries.Empty();
	Simulator.ClearScene();
}

void AAdvPhysScene::ResetPhysObjectsPosition()
{
	for (const auto& Obj : DynamicObjEntries)
	{
		Obj.Comp->SetWorldLocationAndRotation(Obj.Location, Obj.Rotation);
	}
}

void AAdvPhysScene::CopyObjectsToSimulator()
{
	const double StartSeconds = FPlatformTime::Seconds();
	for (const auto& Obj : DynamicObjEntries)
	{
		Simulator.AddDynamicBody(Obj.Comp);
	}

	for (const auto& Obj : StaticObjEntries)
	{
		Simulator.AddStaticBody(Obj.Comp);
	}
	const double Now = FPlatformTime::Seconds();

	FMessageLog("AdvPhysScene").Info(
	FText::Format(
		FText::FromString("CopyObjectsToSimulator took {0}ms, {1} dynamic objects, {2} static objects."),
		(Now - StartSeconds) * 1000,
		DynamicObjEntries.Num(),
		StaticObjEntries.Num()
	));
}

void AAdvPhysScene::Record(const float Interval, const int FrameCount)
{
	FMessageLog("AdvPhysScene").Info(
		FText::Format(
			FText::FromString("Start Recording, {0} objects, {1} frames, {2}s each."),
			DynamicObjEntries.Num(),
			FrameCount,
			Interval
		)
	);
	
	Cancel();
	Status.Current = Recording;
	RecordData = {};
	RecordData.bEnableSOD = bEnableSOD;
	RecordData.HashWorldCenter = GetActorLocation();
	RecordData.HashCellSize = SODHashCellSize;
	
	CopyObjectsToSimulator();
	Simulator.ReserveEvents(FrameCount);

	for (const auto& Actor : EventActors)
	{
		if (!Actor)
		{
			FMessageLog("AdvPhysScene").Error(FText::FromString("EventActors array contains invalid item"));
			continue;
		}
		Simulator.AddEvent(Actor->Time, Actor, Interval, FrameCount);
	}
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
			DynamicObjEntries.Num(),
			RecordData.FrameCount,
			RecordData.FrameInterval
		)
	);

	Cancel();
	Status.Current = Playing;
	Status.PlayStartTime = GetWorld()->GetTimeSeconds();
	Status.PlayLastFrameTime = -1.0f;
	Status.PlayEventActors = EventActors;

	for (const auto& Obj : DynamicObjEntries)
	{
		Obj.Comp->SetSimulatePhysics(false);
	}
	PlayFrame(0.0f);
}

void AAdvPhysScene::PlayRealtimeSimulation()
{
	Cancel();
	UnfreezeDynamicObjects();
	Status.Current = PlayingRealtimeSimulation;
	Status.PlayStartTime = GetWorld()->GetTimeSeconds();
	Status.PlayLastFrameTime = -1.0f;
	Status.PlayEventActors = EventActors;
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

void AAdvPhysScene::FreezeDynamicObjects()
{
	for (const auto& Obj : DynamicObjEntries)
	{
		Obj.Comp->SetSimulatePhysics(false);
	}
}

void AAdvPhysScene::UnfreezeDynamicObjects()
{
	for (const auto& Obj : DynamicObjEntries)
	{
		Obj.Comp->SetSimulatePhysics(true);
	}
}

void AAdvPhysScene::SetPlayFramesPerSecond(float FPS)
{
	PlayFramesPerSecond = FPS;
}

void AAdvPhysScene::ConfigureSimulateOnDemand(const bool bEnabled, const float HashCellSize)
{
	bEnableSOD = bEnabled;
	SODHashCellSize = HashCellSize;
}

EAction AAdvPhysScene::GetAction() const
{
	return Status.Current;
}

float AAdvPhysScene::GetRecordProgress() const
{
	return RecordData.Progress;
}

float AAdvPhysScene::GetPlayFramesPerSecond() const
{
	return PlayFramesPerSecond;
}

int AAdvPhysScene::GetRecordDataFrameCount() const
{
	return RecordData.FrameCount;
}

float AAdvPhysScene::GetRecordDataFrameInterval() const
{
	return RecordData.FrameInterval;
}

bool AAdvPhysScene::GetRecordDataFinished() const
{
	return RecordData.Finished;
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

	const size_t NumOfObjects = DynamicObjEntries.Num();

	// Set location/rotation as-is
	if (!bEnableInterpolation || StartFrame == EndFrame)
	{
		for (int ObjIndex = 0; ObjIndex < NumOfObjects; ObjIndex++)
		{
			const FPhysObjLocRot& StartEntry = RecordData.ObjLocRot[StartFrame * NumOfObjects + ObjIndex];

			DynamicObjEntries[ObjIndex].Comp->SetWorldLocationAndRotationNoPhysics(StartEntry.Location, StartEntry.Rotation);
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

		DynamicObjEntries[ObjIndex].Comp->SetWorldLocationAndRotationNoPhysics(Loc, Rot);
	}
}

void AAdvPhysScene::HandleEventsInFrame(float Time, bool ApplyEventsToRealWorld)
{
	// This is only called when playing recorded bake data.
	// Because playing via Realtime Simulation does not specify its duration -> hence duration unknown. 
	if (!ApplyEventsToRealWorld && Time > GetDuration())
	{
		for (const auto& Actor : Status.PlayEventActors)
		{
			Actor->BroadcastOnTrigger();
		}
		Status.PlayEventActors.Empty();
		return;
	}

	for (int i = Status.PlayEventActors.Num() - 1; i >= 0; i--)
	{
		const auto& E = Status.PlayEventActors[i];
		if (E->Time > Time) continue;
		E->BroadcastOnTrigger();
		if (ApplyEventsToRealWorld)
		{
			E->DoEventUE(DynamicObjEntries);
		}
		Status.PlayEventActors.RemoveAt(i);
	}
}

void AAdvPhysScene::AddTaggedObjects()
{
	int NumOfDynActors = 0, NumOfStaticActors = 0, NumOfDynComps = 0, NumOfStaticComps = 0;
	const double StartSeconds = FPlatformTime::Seconds();
	
	TArray<AActor*> Actors;
	TArray<UStaticMeshComponent*> Comps;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), DynamicTag, Actors);
	for (const auto& Actor : Actors)
	{
		NumOfDynActors++;
		Comps.Empty();
		Actor->GetComponents<UStaticMeshComponent>(Comps);
		for (const auto& Comp : Comps)
		{
			NumOfDynComps++;
			AddDynamicObject(Comp);
		}
	}
		
	Actors.Empty();
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), StaticTag, Actors);
	for (const auto& Actor : Actors)
	{
		NumOfStaticActors++;
		Comps.Empty();
		Actor->GetComponents<UStaticMeshComponent>(Comps);
		for (const auto& Comp : Comps)
		{
			NumOfStaticComps++;
			AddStaticObject(Comp);
		}
	}

	const double Now = FPlatformTime::Seconds();
	
	FMessageLog("AdvPhysScene").Info(
		FText::Format(
			FText::FromString("AddTaggedObjects took {0}ms, Added dynamic: {1}/{2}, static: {3}/{4} (Actors/Components)"),
			(Now - StartSeconds) * 1000,
			NumOfDynActors,
			NumOfDynComps,
			NumOfStaticActors,
			NumOfStaticComps
	));
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
	case PlayingRealtimeSimulation:
		DoPlayRealtimeSimulationTick();
		break;
	default:;
	}
}

void AAdvPhysScene::BeginPlay()
{
	Super::BeginPlay();
	Simulator.Initialize();

	if (bAddTaggedObjectsOnBeginPlay)
	{
		AddTaggedObjects();
	}
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
		RecordFinished.Broadcast();
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
	HandleEventsInFrame(CurrentTime, false);
	Status.PlayLastFrameTime = Now;
	if (CurrentTime > GetDuration())
	{
		FMessageLog("AdvPhysScene").Info(FText::FromString("Playing finished."));
		Status = {};
	}
}

void AAdvPhysScene::DoPlayRealtimeSimulationTick()
{
	const float Now = GetWorld()->GetTimeSeconds();
	const float CurrentTime = Now - Status.PlayStartTime;
	HandleEventsInFrame(CurrentTime, true);
	Status.PlayLastFrameTime = Now;
}

