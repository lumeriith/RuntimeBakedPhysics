// Fill out your copyright notice in the Description page of Project Settings.

#include "PhysSimulator.h"

#include "AdvPhysScene.h"

PhysSimulator::PhysSimulator(): RecordData(nullptr), Foundation(nullptr), Physics(nullptr), Dispatcher(nullptr),
                                Scene(nullptr),
                                Material(nullptr),
                                Pvd(nullptr),
                                bIsInitialized(false),
                                bIsRecording(false),
                                bWantsToStop(false)
{
}

PhysSimulator::~PhysSimulator()
{
}

void PhysSimulator::Initialize()
{
	if (bIsInitialized)
	{
		FMessageLog("PhysSimulator").Error(
			FText::FromString("Tried to initialize twice")
			);
		return;
	}
	FMessageLog("PhysSimulator").Info(
		FText::FromString("Initializing")
		);
	
	Foundation = PxCreateFoundation(PX_FOUNDATION_VERSION, Allocator, ErrorCallback);

	Pvd = PxCreatePvd(*Foundation);
	PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate("localhost", 5425, 10);
	Pvd->connect(*transport,PxPvdInstrumentationFlag::eALL);

	Physics = PxCreatePhysics(PX_PHYSICS_VERSION, *Foundation, PxTolerancesScale(),true,Pvd);

	PxSceneDesc sceneDesc(Physics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
	Dispatcher = PxDefaultCpuDispatcherCreate(2);
	sceneDesc.cpuDispatcher	= Dispatcher;
	sceneDesc.filterShader	= PxDefaultSimulationFilterShader;
	Scene = Physics->createScene(sceneDesc);

	PxPvdSceneClient* pvdClient = Scene->getScenePvdClient();
	if(pvdClient)
	{
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
	}
	Material = Physics->createMaterial(0.5f, 0.5f, 0.6f);

	PxRigidStatic* groundPlane = PxCreatePlane(*Physics, PxPlane(0,1,0,0), *Material);
	Scene->addActor(*groundPlane);
	

	
	for(PxU32 i=0;i<5;i++)
	{
		const PxTransform& t =PxTransform(PxVec3(0,0,stackZ-=10.0f));
		PxU32 size = 10;
		PxReal halfExtent = 2.0f;
		PxShape* shape = Physics->createShape(PxBoxGeometry(halfExtent, halfExtent, halfExtent), *Material);
		for(PxU32 j=0; j<size;j++)
		{
			for(PxU32 k=0;k<size-i;k++)
			{
				PxTransform localTm(PxVec3(PxReal(k*2) - PxReal(size-j), PxReal(j*2+1), 0) * halfExtent);
				PxRigidDynamic* body = Physics->createRigidDynamic(t.transform(localTm));
				body->attachShape(*shape);
				PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
				Scene->addActor(*body);
			}
		}
		shape->release();
	}
	
	bIsInitialized = true;
}


void PhysSimulator::Cleanup()
{
	if (!bIsInitialized)
	{
		FMessageLog("PhysSimulator").Error(
			FText::FromString("Tried to cleanup non-initialized simulator")
			);
		return;
	}
	FMessageLog("PhysSimulator").Info(
		FText::FromString("Cleaning up")
		);
	Scene->release();
	Dispatcher->release();
	Physics->release();	
	PxPvdTransport* transport = Pvd->getTransport();
	Pvd->release();
	transport->release();
	
	Foundation->release();
	
	bIsInitialized = false;
}

void PhysSimulator::ResetScene()
{
	if (!bIsInitialized)
	{
		FMessageLog("PhysSimulator").Error(
			FText::FromString("ResetScene requires PhysSimulator to be initialized")
			);
		return;
	}
}

void PhysSimulator::AddToScene(UStaticMeshComponent* Comp)
{
	if (!bIsInitialized)
	{
		FMessageLog("PhysSimulator").Error(
			FText::FromString("AddToScene requires PhysSimulator to be initialized")
			);
		return;
	}
}

void PhysSimulator::StartRecord(FPhysRecordData* Destination, float RecordInterval, int FrameCount)
{
	if (bIsRecording || !bIsInitialized)
	{
		FMessageLog("PhysSimulator").Error(
			FText::FromString("StartRecord invalid operation.")
		);
		return;
	}
	RecordData = Destination;
	RecordData->Finished = false;
	RecordData->Progress = 0.0f;
	RecordData->FrameCount = FrameCount;
	RecordData->FrameInterval = RecordInterval;
	RecordData->Frames.Empty();
	RecordData->Frames.Reserve(FrameCount);
	RecordData->Frames.AddZeroed(FrameCount);
	bWantsToStop = false;
	RecordThread = std::thread(&PhysSimulator::RecordInternal, this);
	RecordThread.detach();
}

void PhysSimulator::StopRecord()
{
	if (!bIsRecording || !bIsInitialized)
	{
		FMessageLog("PhysSimulator").Error(
			FText::FromString("StopRecord invalid operation.")
		);
		return;
	}
	bWantsToStop = true;
}

bool PhysSimulator::IsInitialized() const
{
	return bIsInitialized;
}

bool PhysSimulator::IsRecording() const
{
	return bIsRecording;
}

void PhysSimulator::RecordInternal()
{
	for (int i = 0; i < RecordData->FrameCount; i++)
	{
		if (bWantsToStop)
		{
			bIsRecording = false;
			return;
		}
		Scene->simulate(RecordData->FrameInterval);
		Scene->fetchResults(true);
		RecordData->Progress = static_cast<float>(i + 1) / RecordData->FrameCount;
	}
	RecordData->Finished = true;
	bIsRecording = false;
}
