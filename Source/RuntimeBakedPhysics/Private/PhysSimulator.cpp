// Fill out your copyright notice in the Description page of Project Settings.
#include "PhysSimulator.h"
#include "AdvPhysScene.h"
#include "PtouConversions.h"

#define PHYSICS_INTERFACE_PHYSX true
#include "PhysXPublicCore.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

PhysSimulator::PhysSimulator(): RecordData(nullptr), Foundation(nullptr), Physics(nullptr), Dispatcher(nullptr),
                                Scene(nullptr),
                                TestMaterial(nullptr),
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

	Physics = PxCreatePhysics(PX_PHYSICS_VERSION, *Foundation, PxTolerancesScale(), true, Pvd);

	CreateSceneInternal();
	
	if (false)
	{
		for(PxU32 i=0;i<5;i++)
		{
			const PxTransform& t =PxTransform(PxVec3(0,0,stackZ-=10.0f));
			PxU32 size = 10;
			PxReal halfExtent = 2.0f;
			PxShape* shape = Physics->createShape(PxBoxGeometry(halfExtent, halfExtent, halfExtent), *TestMaterial);
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

void PhysSimulator::ClearScene()
{
	if (!bIsInitialized)
	{
		FMessageLog("PhysSimulator").Error(
			FText::FromString("ClearScene requires PhysSimulator to be initialized")
			);
		return;
	}

	if (Scene)
	{
		Scene->release();
	}
	
	for (auto& Shape : Shapes)
		Shape.second->release();
	Shapes.clear();

	for (auto& Material : Materials)
		Material->release();
	Materials.clear();

	ObservedBodies.clear();
	
	CreateSceneInternal();
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

	const auto UPhysMat = Comp->GetMaterial(0)->GetPhysicalMaterial();
	
	const uint32 PhysMatId = UPhysMat->GetUniqueID();
	const uint32 MeshId = Comp->GetStaticMesh()->GetUniqueID();
	const uint64 FinalId =  PhysMatId | (MeshId < 32);
	
	if (Shapes.count(FinalId) == 0)
	{
		const auto NewMaterial = Physics->createMaterial(
			UPhysMat->StaticFriction,
			UPhysMat->Friction,
			UPhysMat->Restitution
		);
		Materials.push_back(NewMaterial);

		// const auto& Geom = Comp->GetStaticMesh()->GetBodySetup()->AggGeom;

		const auto UColShape = Comp->GetCollisionShape();
		
		switch (UColShape.ShapeType)
		{
		case ECollisionShape::Line:
			FMessageLog("PhysSimulator").Error(
				FText::FromString("Unsupported Geometry Type: Line")
			);
			break;
		case ECollisionShape::Box:
			Shapes[FinalId] = Physics->createShape(
				PxBoxGeometry(U2PVector(UColShape.GetBox())),
				*NewMaterial);
			break;
		case ECollisionShape::Sphere:
			Shapes[FinalId] = Physics->createShape(
				PxSphereGeometry(UColShape.GetSphereRadius()),
				*NewMaterial);
			break;
		case ECollisionShape::Capsule:
			Shapes[FinalId] = Physics->createShape(
				PxCapsuleGeometry(UColShape.GetCapsuleRadius(), UColShape.GetCapsuleHalfHeight()),
				*NewMaterial);
			break;
		default: ;
		}
	}
	
	const auto& PShape = Shapes[FinalId];

	PxVec3 PLoc = U2PVector(Comp->GetComponentLocation());
	PxQuat PQuat = U2PQuat(Comp->GetComponentRotation().Quaternion());
	
	const PxTransform& PTransform = PxTransform(PLoc, PQuat);
	PxRigidDynamic* body = Physics->createRigidDynamic(PTransform);
	body->attachShape(*PShape);
	PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
	Scene->addActor(*body);
	ObservedBodies.push_back(body);
}

void PhysSimulator::StartRecord(FPhysRecordData* Destination, float RecordInterval, int FrameCount, float GravityZ)
{
	if (bIsRecording || !bIsInitialized)
	{
		FMessageLog("PhysSimulator").Error(
			FText::FromString("StartRecord invalid operation.")
		);
		return;
	}
	
	Scene->setGravity(PxVec3(0.0f, 0.0f, GravityZ));
	
	RecordData = Destination;
	RecordData->Finished = false;
	RecordData->Progress = 0.0f;
	RecordData->FrameCount = FrameCount;
	RecordData->FrameInterval = RecordInterval;
	RecordData->ObjLocRot.Empty();
	RecordData->ObjLocRot.Reserve(FrameCount * ObservedBodies.size());
	RecordData->ObjLocRot.AddZeroed(FrameCount * ObservedBodies.size());
	
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
		for (int j = 0; j < ObservedBodies.size(); j++)
		{
			auto& Frame = RecordData->ObjLocRot[i * ObservedBodies.size() + j];
			const auto& Pose = ObservedBodies[j]->getGlobalPose();
			Frame.Location = P2UVector(Pose.p);
			Frame.Rotation = UE::Math::TRotator(P2UQuat(Pose.q));
		}
		RecordData->Progress = static_cast<float>(i + 1) / RecordData->FrameCount;
	}
	RecordData->Finished = true;
	bIsRecording = false;
}

void PhysSimulator::CreateSceneInternal()
{
	PxSceneDesc SceneDesc(Physics->getTolerancesScale());
	Dispatcher = PxDefaultCpuDispatcherCreate(2);
	SceneDesc.cpuDispatcher	= Dispatcher;
	SceneDesc.filterShader	= PxDefaultSimulationFilterShader;
	Scene = Physics->createScene(SceneDesc);
	
	PxPvdSceneClient* PvdClient = Scene->getScenePvdClient();
	if (PvdClient)
	{
		PvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
		PvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
		PvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
	}
	TestMaterial = Physics->createMaterial(0.5f, 0.5f, 0.6f);
	
	PxRigidStatic* groundPlane = PxCreatePlane(*Physics, PxPlane(0,0,1,0), *TestMaterial);
	Scene->addActor(*groundPlane);
}
