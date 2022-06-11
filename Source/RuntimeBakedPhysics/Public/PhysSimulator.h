// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <thread>
#include "AdvPhysDataTypes.h"

#include "ThirdParty/PhysX3/PhysX_3.4/Include/PxPhysics.h"
#include "ThirdParty/PhysX3/PhysX_3.4/Include/PxPhysicsAPI.h"

using namespace physx; 

struct PhysCompoundShape
{
	std::vector<PxShape*> Shapes;
};

class RUNTIMEBAKEDPHYSICS_API PhysSimulator
{
public:
	PhysSimulator();
	~PhysSimulator();

	void Initialize();
	void Cleanup();

	// Events
	void ReserveEvents(int FrameCount);
	void AddEvents(TArray<std::tuple<float, std::any>>& Pairs, float Interval, int FrameCount);
	void AddEvent(float Time, std::any Event, float Interval, int FrameCount);
	void FreeEvents();

	// Scene-Related
	void ClearScene();
	
	void AddStaticBody(UStaticMeshComponent* Comp, bool bUseSimpleGeometry = false);
	void AddDynamicBody(UStaticMeshComponent* Comp, bool bUseSimpleGeometry = false);

	void StartRecord(FPhysRecordData* Destination, float RecordInterval, int FrameCount, float GravityZ);
	void StopRecord();
	// Others
	bool IsInitialized() const;
	bool IsRecording() const;
protected:
	void RecordInternal();
	void HandleEventsInternal(int Frame);
	void HandleExplosionEventInternal(FPhysEvent_Explosion&& Explosion);
	
	void CreateSceneInternal();

	void GetShapeInternal(const UStaticMeshComponent* Comp, bool bUseSimpleGeometry, PhysCompoundShape& OutShape);
	PxGeometry GetSimpleGeometry(const UStaticMeshComponent* Comp) const;
	PxConvexMesh* GetConvexMeshInternal(UStaticMesh* Mesh, int ConvexElemIndex);

	FPhysRecordData* RecordData;

	PxDefaultAllocator		Allocator;
	PxDefaultErrorCallback	ErrorCallback;

	PxFoundation*			Foundation;
	PxPhysics*				Physics;

	PxDefaultCpuDispatcher*	Dispatcher;
	PxScene*				Scene;

	PxPvd*                  Pvd;

	PxCooking*				Cooking;

	PxReal stackZ = 10.0f;
	
	bool bIsInitialized;
	bool bIsRecording;
	bool bWantsToStop;
	std::thread RecordThread;
	
	std::unordered_map<uint64, PxConvexMesh*> ConvexMeshes;
	std::vector<PxRigidDynamic*> ObservedBodies;

	std::vector<std::shared_ptr<FPhysEventNode>> Events;
};