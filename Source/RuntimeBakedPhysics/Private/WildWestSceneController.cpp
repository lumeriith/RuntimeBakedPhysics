// Fill out your copyright notice in the Description page of Project Settings.


#include "WildWestSceneController.h"
#include <algorithm>

void AWildWestSceneController::BeginRecordScene(PhysSimulator* Sim)
{
	Super::BeginRecordScene(Sim);

	for (int i = 0; i < Sim->ObservedBodies.size(); i++)
	{
		Sim->ObservedBodies[i]->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
		Indices.push_back(i);
		std::sort(Indices.begin(), Indices.end(), 
                 [Sim](const int& a, const int& b) -> bool
             { 
                 return Sim->ObservedBodies[a]->getGlobalPose().p.z < Sim->ObservedBodies[b]->getGlobalPose().p.z; 
             });
	}
}

void AWildWestSceneController::RecordSceneTick(PhysSimulator* Sim, int FrameIndex)
{
	Super::RecordSceneTick(Sim, FrameIndex);

	if (FrameIndex >= EndFrame)
	{
		if (Indices.size() == 0) return;
		for (int i = 0; i < Indices.size(); i++)
		{
			const auto BodyIndex = Indices[i];
			Sim->ObservedBodies[BodyIndex]->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, false);
		}
		Indices.clear();
		return;
	}
	
	float Normalized = static_cast<float>(FrameIndex - StartFrame) / EndFrame;
	Normalized = FMath::Clamp(Normalized, 0.f, 1.f);
	const int NumOfBody = Sim->ObservedBodies.size();
	const int DesiredKinematicNum = NumOfBody - FMath::RoundToInt(Normalized * NumOfBody);

	for (int i = 0; i < Indices.size() - DesiredKinematicNum; i++)
	{
		const auto Index = 0; //FMath::RandRange(0, Indices.size() - 1);
		const auto BodyIndex = Indices[Index];
		Indices.erase(Indices.begin() + Index);
		Sim->ObservedBodies[BodyIndex]->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, false);
	}
}
