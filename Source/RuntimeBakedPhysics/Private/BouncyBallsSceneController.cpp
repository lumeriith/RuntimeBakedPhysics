// Fill out your copyright notice in the Description page of Project Settings.


#include "BouncyBallsSceneController.h"

#include "AdvPhysScene.h"
#include "Components/AudioComponent.h"
#include "Particles/ParticleEmitter.h"
#include "Particles/ParticleSystemComponent.h"

void ABouncyBallsSceneController::BeginRecordScene(PhysSimulator* Sim)
{
	Super::BeginRecordScene(Sim);
	for (const auto& B : Sim->ObservedBodies)
	{
		B->setLinearVelocity(PxVec3(FMath::RandRange(-Magnitude, Magnitude), FMath::RandRange(-Magnitude, Magnitude), FMath::RandRange(-Magnitude, Magnitude)));
	}
}

void ABouncyBallsSceneController::BeginPlayScene(AAdvPhysScene* Scene)
{
	Super::BeginPlayScene(Scene);
	if (Materials.size() == 0)
	{
		Materials.reserve(Scene->DynamicObjEntries.Num());
		for (const auto& E : Scene->DynamicObjEntries)
		{
			Materials.push_back(E.Comp->GetMaterial(0));
		}
		return;
	}

	for (int i = 0; i < Scene->DynamicObjEntries.Num(); i++)
	{
		Scene->DynamicObjEntries[i].Comp->SetMaterial(0, Materials[i]);
	}
}

void ABouncyBallsSceneController::DidStartSimulateOnDemand(AAdvPhysScene* Scene, int ObjIndex, int FrameIndex)
{
	Super::DidStartSimulateOnDemand(Scene, ObjIndex, FrameIndex);
	if (!bVisualizeSoD) return;
	const auto& Comp = Scene->DynamicObjEntries[ObjIndex].Comp;
	Comp->SetMaterial(0, ReplacedMaterial);
	TArray<UParticleSystemComponent*> Particles;
	TArray<UAudioComponent*> Audios;
	
	Comp->GetOwner()->GetComponents(Particles);
	Comp->GetOwner()->GetComponents(Audios);

	for (const auto E : Particles)
	{
		E->Activate();
	}

	for (const auto A : Audios)
	{
		A->Play();
	}
}
