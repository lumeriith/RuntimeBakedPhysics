// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <thread>
#include "AdvPhysDataTypes.h"

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
	void ClearScene();
	void AddToScene(UStaticMeshComponent* Comp, bool );

	void StartRecord(FPhysRecordData* Destination, float RecordInterval, int FrameCount, float GravityZ);
	void StopRecord();
	// Others
	bool IsInitialized() const;
	bool IsRecording() const;
protected:
	void RecordInternal();
	void CreateSceneInternal();

	FPhysRecordData* RecordData;

	PxDefaultAllocator		Allocator;
	PxDefaultErrorCallback	ErrorCallback;

	PxFoundation*			Foundation;
	PxPhysics*				Physics;

	PxDefaultCpuDispatcher*	Dispatcher;
	PxScene*				Scene;

	PxMaterial*				TestMaterial;

	PxPvd*                  Pvd;

	PxReal stackZ = 10.0f;
	
	bool bIsInitialized;
	bool bIsRecording;
	bool bWantsToStop;
	std::thread RecordThread;

	std::unordered_map<uint64, PxShape*> Shapes;
	std::vector<PxMaterial*> Materials;
	std::vector<PxRigidDynamic*> ObservedBodies;
};
