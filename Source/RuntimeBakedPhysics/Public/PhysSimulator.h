// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <thread>
#include "AdvPhysDataTypes.h"
#include "AdvPhysSceneController.h"

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
	void AddEvent(float Time, AAdvPhysEventBase* Event, float Interval, int FrameCount);
	void FreeEvents();

	// Scene-Related
	void ClearScene();
	
	void AddStaticBody(UStaticMeshComponent* Comp, EShapeType Type);
	void AddDynamicBody(UStaticMeshComponent* Comp, bool bUseSimpleGeometry = false);

	void StartRecord(FPhysRecordData* Destination, float RecordInterval, int FrameCount, float GravityZ);
	void StopRecord();
	// Others
	bool IsInitialized() const;
	bool IsRecording() const;
	
	class AAdvPhysSceneController* Controller;

	inline static PxDefaultAllocator		Allocator;
	inline static PxDefaultErrorCallback	ErrorCallback;

	inline static PxFoundation*				Foundation;
	inline static PxPhysics*				Physics;

	inline static PxDefaultCpuDispatcher*	Dispatcher;
	inline static PxPvd*						Pvd;
	inline static PxCooking*				Cooking;

	PxScene* Scene;

	std::unordered_map<uint64, PxConvexMesh*> ConvexMeshes;
	std::vector<PxRigidDynamic*> ObservedBodies;

	std::vector<std::shared_ptr<FPhysEventNode>> Events;

	FPhysRecordData* RecordData;
	
protected:
	void RecordInternal();
	void HandleEventsInternal(int Frame);
	
	void CreateSceneInternal();

	void GetShapeInternal(const UStaticMeshComponent* Comp, EShapeType Type, PhysCompoundShape& OutShape);
	std::shared_ptr<PxGeometry> GetSimpleGeometry(const UStaticMeshComponent* Comp) const;
	PxConvexMesh* GetConvexMeshInternal(UStaticMesh* Mesh, int ConvexElemIndex);
	
	inline static int StaticRefCount = 0;
	
	bool bIsInitialized;
	bool bIsRecording;
	bool bWantsToStop;
	std::thread RecordThread;
};