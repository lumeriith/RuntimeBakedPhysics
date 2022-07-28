// Fill out your copyright notice in the Description page of Project Settings.


#include "ElvenRuinsSceneController.h"

void AElvenRuinsSceneController::BeginRecordScene(PhysSimulator* Sim)
{
	Super::BeginRecordScene(Sim);

	for (int i = 0; i < Sim->ObservedBodies.size(); i++)
	{
		Sim->ObservedBodies[i]->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
	}
}
