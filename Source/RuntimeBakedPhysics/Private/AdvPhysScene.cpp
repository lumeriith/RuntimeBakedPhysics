// Fill out your copyright notice in the Description page of Project Settings.


#include "AdvPhysScene.h"

#include "AdvPhysHashHelper.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AAdvPhysScene::AAdvPhysScene()
{
	SetRootComponent(CreateDefaultSubobject<USceneComponent>("Scene Root Component"));
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
		Obj.Comp->SetCollisionProfileName(Obj.CollisionProfile);
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

void AAdvPhysScene::DrawSODObjectBounds()
{
	if (Status.Current != Playing) return;
	
	const float Now = GetWorld()->GetTimeSeconds();
	const float CurrentTime = Now - Status.PlayStartTime;

	int FrameIndex = FMath::FloorToInt(CurrentTime / RecordData.FrameInterval);
	if (FrameIndex >= RecordData.FrameCount)
		FrameIndex = RecordData.FrameCount - 1;
	const size_t NumOfObjects = DynamicObjEntries.Num();

	for (int i = 0; i < NumOfObjects; i++)
	{
		const auto& Frame = RecordData.ObjSOD[FrameIndex * NumOfObjects + i];
		const auto BoundsCenter = Frame.Bounds.GetCenter();
		const auto BoundsExtent = Frame.Bounds.GetExtent();
		DrawDebugBox(GetWorld(), BoundsCenter, BoundsExtent, FColor::Green, false, 0);
	}
}

void AAdvPhysScene::DrawSODHashCubes()
{
	if (Status.Current != Playing) return;
	
	const float Now = GetWorld()->GetTimeSeconds();
	const float CurrentTime = Now - Status.PlayStartTime;

	int FrameIndex = FMath::FloorToInt(CurrentTime / RecordData.FrameInterval);
	if (FrameIndex >= RecordData.FrameCount)
		FrameIndex = RecordData.FrameCount - 1;
	const size_t NumOfObjects = DynamicObjEntries.Num();

	for (int i = 0; i < NumOfObjects; i++)
	{
		const auto& Frame = RecordData.ObjSOD[FrameIndex * NumOfObjects + i];
		unsigned StartXIndex, StartYIndex, StartZIndex, EndXIndex, EndYIndex, EndZIndex;
		AdvPhysHashHelper::SplitFromHash(Frame.StartHash, StartXIndex, StartYIndex, StartZIndex);
		AdvPhysHashHelper::SplitFromHash(Frame.EndHash, EndXIndex, EndYIndex, EndZIndex);

		const double OffsetMetersFromCenter = RecordData.HashCellSize * WORLD_CELL_LENGTH / 2.0f;
		FVector CellWorldStart = RecordData.HashWorldCenter - OffsetMetersFromCenter * FVector::OneVector;
		
		FVector MinPoint = CellWorldStart
			+ StartXIndex * FVector::ForwardVector * RecordData.HashCellSize
			+ StartYIndex * FVector::RightVector * RecordData.HashCellSize
			+ StartZIndex * FVector::UpVector * RecordData.HashCellSize;
		FVector MaxPoint = CellWorldStart
			+ EndXIndex * FVector::ForwardVector * RecordData.HashCellSize
			+ EndYIndex * FVector::RightVector * RecordData.HashCellSize
			+ EndZIndex * FVector::UpVector * RecordData.HashCellSize
			+ FVector::OneVector * RecordData.HashCellSize;
		
		const auto BoundsCenter = (MinPoint + MaxPoint) / 2;
		const auto BoundsExtent = (MaxPoint - MinPoint) / 2;
		DrawDebugSolidBox(GetWorld(), BoundsCenter, BoundsExtent, FColor(255, 0, 0, 30), false, 0);
	}
}

void AAdvPhysScene::DrawSODActivatedObjects()
{
	const size_t NumOfObjects = DynamicObjEntries.Num();
	for (int ObjIndex = 0; ObjIndex < NumOfObjects; ObjIndex++)
	{
		if (!Status.SODActivationState[ObjIndex]) continue;
		const auto& Comp = DynamicObjEntries[ObjIndex].Comp;
		const auto& Bounds = Comp->Bounds;
		DrawDebugBox(GetWorld(), Bounds.Origin, Bounds.BoxExtent, FColor(255, 255, 0, 100), false, 0);
	}
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
	Status.LastPlayFrameTime = -1.0f;
	Status.PlayEventActors = EventActors;
	
	if (RecordData.bEnableSOD)
	{
		Status.SODActivationState.AddZeroed(DynamicObjEntries.Num());
		Status.LastSODCheckTime = -1.0f;
	}
	
	for (const auto& Obj : DynamicObjEntries)
	{
		Obj.Comp->SetSimulatePhysics(false);
		Obj.Comp->SetCollisionProfileName(TEXT("OverlapAll"));
	}
	PlayFrame(0.0f);
}

void AAdvPhysScene::PlayRealtimeSimulation()
{
	Cancel();
	UnfreezeDynamicObjects();
	Status.Current = PlayingRealtimeSimulation;
	Status.PlayStartTime = GetWorld()->GetTimeSeconds();
	Status.LastPlayFrameTime = -1.0f;
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
	const float Frame = Time / RecordData.FrameInterval;
	int StartFrame = FMath::FloorToInt(Frame);
	int EndFrame = FMath::CeilToInt(Frame);

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
			if (RecordData.bEnableSOD && Status.SODActivationState[ObjIndex]) continue;
			const FPhysObjLocRot& StartEntry = RecordData.ObjLocRot[StartFrame * NumOfObjects + ObjIndex];

			DynamicObjEntries[ObjIndex].Comp->SetWorldLocationAndRotationNoPhysics(StartEntry.Location, StartEntry.Rotation);
		}
		return;
	}

	// Interpolate between two frames
	const float Value = Frame - StartFrame;
	for (int ObjIndex = 0; ObjIndex < NumOfObjects; ObjIndex++)
	{
		if (RecordData.bEnableSOD && Status.SODActivationState[ObjIndex]) continue;
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

void AAdvPhysScene::CheckSODAtTime(float Time)
{
	const float Frame = Time / RecordData.FrameInterval;
	int FrameIndex = FMath::FloorToInt(Frame);
	if (FrameIndex >= RecordData.FrameCount)
		FrameIndex = RecordData.FrameCount - 1;
	
	const auto NumOfObjects = DynamicObjEntries.Num();
	if (bUseNaiveSODCheck)
	{
		for (int i = 0; i < NumOfObjects; i++)
		{
			if (Status.SODActivationState[i]) continue;

			for (const auto& Act : OriginalActivators)
			{
				if (!CheckActivatorIntersect(Act, RecordData.ObjSOD[FrameIndex * NumOfObjects + i], true)) continue;
				SimulateObjectOnDemand(i, FrameIndex);
				break;
			}

			// Don't use iterator as items are added while iteration
			const auto Num = Status.AddedActivators.Num();
			for (int j = 0; j < Num; j++)
			{
				const auto& Act = Status.AddedActivators[j];
				if (!CheckActivatorIntersect(Act, RecordData.ObjSOD[FrameIndex * NumOfObjects + i], false)) continue;
				SimulateObjectOnDemand(i, FrameIndex);
				break;
			}
		}
		return;
	}
	
	RebuildSODMap(FrameIndex);

	for (const auto& Act : OriginalActivators)
	{
		CheckFromSODMap(Act, FrameIndex, true);
	}
	const auto Num = Status.AddedActivators.Num();
	for (int i = 0; i < Num; i++)
	{
		const auto& Act = Status.AddedActivators[i];
		CheckFromSODMap(Act, FrameIndex, false);
	}
}

bool AAdvPhysScene::CheckActivatorIntersect(const USceneComponent* Activator, const FPhysObjSODData& SODData, const bool bIsOriginal) const
{
	const auto ActBox = Activator->Bounds.GetBox();
	return ActBox.ExpandBy(bIsOriginal ? SODOriginalActivatorBoundExpansion : SODAddedActivatorBoundExpansion).Intersect(SODData.Bounds);
}

void AAdvPhysScene::RebuildSODMap(int FrameIndex)
{
	auto& Map = Status.SODMap;
	Map.clear();
	const auto NumOfObjects = DynamicObjEntries.Num();
	
	for (int i = 0; i < NumOfObjects; i++)
	{
		if (Status.SODActivationState[i]) continue;;
		const auto& SODData = RecordData.ObjSOD[FrameIndex * NumOfObjects + i];
		
		auto UpdateMap = [&Map, &i](uint32 Hash)
		{
			const auto FindIter = Map.find(Hash);
			if (FindIter != Map.end())
			{
				FindIter->second.push_back(i);
				return;
			}
			Map.insert(std::hash_map<uint32, std::list<int>>::value_type(Hash, std::list<int>()));
			
		};
		AdvPhysHashHelper::CubicSweepHash(SODData.StartHash, SODData.EndHash, UpdateMap);
	}
}

void AAdvPhysScene::CheckFromSODMap(const USceneComponent* Activator, const int FrameIndex, const bool bIsOriginal)
{
	const auto NumOfObjects = DynamicObjEntries.Num();
	uint32 StartHash, EndHash;
	AdvPhysHashHelper::GetHash(Activator->Bounds.GetBox().ExpandBy(
		bIsOriginal ? SODOriginalActivatorBoundExpansion : SODAddedActivatorBoundExpansion),
		RecordData.HashWorldCenter, RecordData.HashCellSize,
		StartHash, EndHash);
		
	auto CheckForActivation = [&](uint32 Hash)
	{
		const auto& Map = Status.SODMap;
		const auto FindIter = Map.find(Hash);
		if (FindIter == Map.end()) return;
		for (const int ObjIndex : FindIter->second)
		{
			if (Status.SODActivationState[ObjIndex]) continue;
			if (!CheckActivatorIntersect(Activator, RecordData.ObjSOD[FrameIndex * NumOfObjects + ObjIndex], bIsOriginal)) continue;
			SimulateObjectOnDemand(ObjIndex, FrameIndex);
		}
	};
	AdvPhysHashHelper::CubicSweepHash(StartHash, EndHash, CheckForActivation);
}

void AAdvPhysScene::SimulateObjectOnDemand(int ObjIndex, int FrameIndex)
{
	Status.SODActivationState[ObjIndex] = true;
	
	int StartFrameIndex = FrameIndex - 1;
	int EndFrameIndex = FrameIndex;
	if (StartFrameIndex < 0)
	{
		StartFrameIndex = 0;
		EndFrameIndex = 1;
	}

	const auto NumOfObjects = DynamicObjEntries.Num();

	const auto& StartFrame = RecordData.ObjLocRot[StartFrameIndex * NumOfObjects + ObjIndex];
	const auto& EndFrame = RecordData.ObjLocRot[EndFrameIndex * NumOfObjects + ObjIndex];
	const auto& Comp = DynamicObjEntries[ObjIndex].Comp;
	
	Comp->SetSimulatePhysics(true);
	Comp->SetCollisionProfileName(DynamicObjEntries[ObjIndex].CollisionProfile);
	
	Comp->SetWorldLocationAndRotation(StartFrame.Location, StartFrame.Rotation, false, nullptr, ETeleportType::ResetPhysics);
	Comp->SetWorldLocationAndRotation(EndFrame.Location, EndFrame.Rotation, true, nullptr, ETeleportType::ResetPhysics);

	const auto LinearVel = (EndFrame.Location - StartFrame.Location) / RecordData.FrameInterval;
	const auto AngularVel = (EndFrame.Rotation - StartFrame.Rotation).Euler() / RecordData.FrameInterval;
	Comp->SetPhysicsLinearVelocity(LinearVel);
	Comp->SetPhysicsAngularVelocityInDegrees(AngularVel);
	
	if (bEnableSODChainReaction)
	{
		Status.AddedActivators.Add(Comp->GetAttachmentRoot());
	}
}

void AAdvPhysScene::AddTaggedObjects()
{
	int NumOfDynActors = 0, NumOfStaticActors = 0, NumOfDynComps = 0, NumOfStaticComps = 0, NumOfActivators = 0;
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

	Actors.Empty();
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), ActivatorTag, Actors);
	for (const auto& Actor : Actors)
	{
		NumOfActivators++;
		OriginalActivators.Add(Actor->GetRootComponent());
	}

	const double Now = FPlatformTime::Seconds();
	
	FMessageLog("AdvPhysScene").Info(
		FText::Format(
			FText::FromString("AddTaggedObjects took {0}ms, Added dynamic: {1}/{2}, static: {3}/{4} (Actors/Components), activators: {5}"),
			(Now - StartSeconds) * 1000,
			NumOfDynActors,
			NumOfDynComps,
			NumOfStaticActors,
			NumOfStaticComps,
			NumOfActivators
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

	if (Status.Current == Playing && RecordData.bEnableSOD)
	{
		if (bDrawSODObjectBoundsOnPlay)
			DrawSODObjectBounds();
		if (bDrawSODHashCubesOnPlay)
			DrawSODHashCubes();
		if (bDrawSODActivatedObjectsOnPlay)
			DrawSODActivatedObjects();
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
	const float CurrentTime = Now - Status.PlayStartTime;
	if (RecordData.bEnableSOD && (SODCheckFramesPerSecond <= 0 || Now - Status.LastSODCheckTime >= 1.0f / SODCheckFramesPerSecond))
	{
		CheckSODAtTime(CurrentTime);
		Status.LastSODCheckTime = Now;
	}
	
	if (PlayFramesPerSecond <= 0 || Now - Status.LastPlayFrameTime >= 1.0f / PlayFramesPerSecond)
	{
		PlayFrame(CurrentTime);
		HandleEventsInFrame(CurrentTime, false);
		Status.LastPlayFrameTime = Now;
		if (CurrentTime > GetDuration())
		{
			FMessageLog("AdvPhysScene").Info(FText::FromString("Playing finished."));
			Status = {};
		}
	}
}

void AAdvPhysScene::DoPlayRealtimeSimulationTick()
{
	const float Now = GetWorld()->GetTimeSeconds();
	const float CurrentTime = Now - Status.PlayStartTime;
	HandleEventsInFrame(CurrentTime, true);
	Status.LastPlayFrameTime = Now;
}

