// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <thread>
#include "CoreMinimal.h"
#include "AdvPhysDataTypes.h"

#include "PhysicsPublic.h"
#include "PhysXPublic.h"
#include "Runtime/Engine/Private/PhysicsEngine/PhysXSupport.h"
#include "PhysXIncludes.h"


#include "ThirdParty/PhysX3/PhysX_3.4/Include/PxPhysics.h"
#include "ThirdParty/PhysX3/PhysX_3.4/Include/PxPhysicsAPI.h"

using namespace physx;

class RUNTIMEBAKEDPHYSICS_API PhysSimulator
{
public:
	PhysSimulator();
	~PhysSimulator();

	void Initialize();
	void Cleanup();

	// Scene-Related
	void ResetScene();
	void AddToScene(UStaticMeshComponent* Comp);

	void StartRecord(FPhysRecordData* Destination, float RecordInterval, int FrameCount);
	void StopRecord();
	// Others
	bool IsInitialized() const;
	bool IsRecording() const;
protected:
	void RecordInternal();

	FPhysRecordData* RecordData;

	PxDefaultAllocator		gAllocator;
	PxDefaultErrorCallback	gErrorCallback;

	PxFoundation*			gFoundation;
	PxPhysics*				gPhysics;

	PxDefaultCpuDispatcher*	gDispatcher;
	PxScene*				gScene;

	PxMaterial*				gMaterial;

	PxPvd*                  gPvd;

	PxReal stackZ = 10.0f;
	
	bool bIsInitialized;
	bool bIsRecording;
	bool bWantsToStop;
	std::thread RecordThread;
};
