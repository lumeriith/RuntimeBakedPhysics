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
	Status.RecordCursor++;
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
	PlayFrame(0.0f);
}

void AAdvPhysScene::Cancel()
{
	ResetObjects();
	Status = {};
}

void AAdvPhysScene::DuplicateToSubWorld()
{
	if (!SubWorld)
	{
		FMessageLog("AdvPhysScene").Error(FText::FromString("SubWorld not found!"));
		return;
	}

	for (const auto& Obj : PhysObjects)
	{
		auto NewComp =
			NewObject<UStaticMeshComponent>(SubWorldActor, UStaticMeshComponent::StaticClass());
		NewComp->RegisterComponent();
		NewComp->AttachToComponent(SubWorldActor->Scene,
			FAttachmentTransformRules::SnapToTargetIncludingScale);
		NewComp->SetStaticMesh(Obj.Comp->GetStaticMesh());
		NewComp->SetMaterial(0, Obj.Comp->GetMaterial(0));
		SubWorldComps.Add(NewComp);
	}

	FMessageLog("AdvPhysScene").Info(
		FText::Format(
			FText::FromString("Duplicated {0} physics objects to sub-world actor. (Now total {1})"),
			PhysObjects.Num(),
			SubWorldComps.Num()
		)
	);
}

void AAdvPhysScene::ClearSubWorld()
{
	if (!SubWorld)
	{
		FMessageLog("AdvPhysScene").Error(FText::FromString("SubWorld not found!"));
		return;
	}

	for (const auto& Comp : SubWorldComps)
	{
		Comp->DestroyComponent();
	}

	FMessageLog("AdvPhysScene").Info(
		FText::Format(
			FText::FromString("Cleared {0} physics objects in sub-world actor."),
			SubWorldComps.Num()
		)
	);
	
	SubWorldComps.Empty();
}

bool AAdvPhysScene::IsRecordCursorAtEnd() const
{
	return Status.RecordCursor >= RecordData.FrameCount;
}

float AAdvPhysScene::GetDuration() const
{
	return RecordData.FrameCount * RecordData.FrameInterval;
}


void AAdvPhysScene::RecordFrame()
{
	if (IsRecordCursorAtEnd())
	{
		FMessageLog("AdvPhysScene").Error(FText::FromString("Current frame out of bounds!"));
		return;
	}

	const size_t NumOfObjects = PhysObjects.Num();
	for (int ObjIndex = 0; ObjIndex < NumOfObjects; ObjIndex++)
	{
		const FVector Loc = PhysObjects[ObjIndex].Comp->GetComponentLocation();
		const FRotator Rot = PhysObjects[ObjIndex].Comp->GetComponentRotation();

		FPhysEntry& Entry = RecordData.Entries[Status.RecordCursor * NumOfObjects + ObjIndex];
		Entry.Location = Loc;
		Entry.Rotation = Rot;
	}
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
			const FPhysEntry& StartEntry = RecordData.Entries[StartFrame * NumOfObjects + ObjIndex];

			PhysObjects[ObjIndex].Comp->SetWorldLocationAndRotationNoPhysics(StartEntry.Location, StartEntry.Rotation);
		}
		return;
	}

	// Interpolate between two frames
	const float Value = frame - StartFrame;
	for (int ObjIndex = 0; ObjIndex < NumOfObjects; ObjIndex++)
	{
		const FPhysEntry& StartEntry = RecordData.Entries[StartFrame * NumOfObjects + ObjIndex];
		const FPhysEntry& EndEntry = RecordData.Entries[EndFrame * NumOfObjects + ObjIndex];

		const FVector Loc = StartEntry.Location * (1.0f - Value) + EndEntry.Location * Value;
		const FRotator Rot = FMath::Lerp(StartEntry.Rotation, EndEntry.Rotation, Value);

		PhysObjects[ObjIndex].Comp->SetWorldLocationAndRotationNoPhysics(Loc, Rot);
	}
}

bool AAdvPhysScene::ShouldRecordFrame() const
{
	if (IsRecordCursorAtEnd()) return false;

	const float DesiredGameTime = Status.StartTime + Status.RecordCursor * RecordData.FrameInterval;
	return GetWorld()->GetTimeSeconds() >= DesiredGameTime;
}

// Called every frame
void AAdvPhysScene::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	switch (Status.Current)
	{
	case Idle: break;
	case Recording:
		TickRecording();
		break;
	case Playing:
		TickPlaying();
		break;
	default:;
	}
}


void AAdvPhysScene::BeginPlay()
{
	Super::BeginPlay();
	if (bEnableSubWorld)
	{
		FMessageLog("AdvPhysScene").Info(FText::FromString("Creating SubWorld"));
		const auto MainWorld = GetWorld();
		SubWorld = UWorld::CreateWorld(EWorldType::Editor, true, TEXT("SubWorld"));

		// Prevent crash on BeginPlay
		auto& CurContext = GEngine->CreateNewWorldContext(EWorldType::Editor);
		CurContext.SetCurrentWorld(SubWorld);

		// SubWorld->SetGameInstance(MainWorld->GetGameInstance());
		// SubWorld->SetGameMode(MainWorld->URL);
		SubWorld->bShouldSimulatePhysics = true;

		SubWorld->InitializeActorsForPlay(MainWorld->URL);
		SubWorld->BeginPlay();
		SubWorldActor = SubWorld->SpawnActor<AAdvPhysParent>(AAdvPhysParent::StaticClass());
		SubWorldActor->RegisterAllActorTickFunctions(true, true);
	}
}

void AAdvPhysScene::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	if (SubWorld)
	{
		FMessageLog("AdvPhysScene").Info(FText::FromString("Destroying SubWorld"));
		SubWorld->DestroyWorld(true);
	}
}

void AAdvPhysScene::TickRecording()
{
	if (!ShouldRecordFrame()) return;
	RecordFrame();
	Status.RecordCursor++;
	if (IsRecordCursorAtEnd())
	{
		FMessageLog("AdvPhysScene").Info(FText::FromString("Recording finished."));
		Status = {};
		StopSimulation();
	}
}

void AAdvPhysScene::TickPlaying()
{
	const float currentTime = GetWorld()->GetTimeSeconds() - Status.StartTime;

	PlayFrame(currentTime);

	if (currentTime > GetDuration())
	{
		FMessageLog("AdvPhysScene").Info(FText::FromString("Playing finished."));
		Status = {};
	}
}

