// Fill out your copyright notice in the Description page of Project Settings.
#include "PhysSimulator.h"

#include "AdvPhysHashHelper.h"
#include "AdvPhysScene.h"
#include "PtouConversions.h"

#include "PhysXPublicCore.h"
#include "Chaos/TriangleMeshImplicitObject.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

PhysSimulator::PhysSimulator(): RecordData(nullptr), Scene(nullptr), bIsInitialized(false), bIsRecording(false),
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

	StaticRefCount++;
	if (StaticRefCount == 1)
	{
		FMessageLog("PhysSimulator").Info(FText::FromString("Preparing Static PhysX Components"));
		Foundation = PxCreateFoundation(PX_FOUNDATION_VERSION, Allocator, ErrorCallback);
		Pvd = PxCreatePvd(*Foundation);
		PxPvdTransport* Transport = PxDefaultPvdSocketTransportCreate("localhost", 5425, 10);
		Pvd->connect(*Transport,PxPvdInstrumentationFlag::eALL);
		Physics = PxCreatePhysics(PX_PHYSICS_VERSION, *Foundation, PxTolerancesScale(), true, Pvd);
		Cooking = PxCreateCooking(PX_PHYSICS_VERSION, *Foundation, PxCookingParams(Physics->getTolerancesScale()));
	}
	
	CreateSceneInternal();
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
	StaticRefCount--;
	if (StaticRefCount == 0)
	{
		FMessageLog("PhysSimulator").Info(FText::FromString("Releasing Static PhysX Components"));
		Dispatcher->release();
		Physics->release();	
		PxPvdTransport* transport = Pvd->getTransport();
		Pvd->release();
		transport->release();
		Cooking->release();
		Foundation->release();
	}
	
	bIsInitialized = false;
}

void PhysSimulator::ReserveEvents(int FrameCount)
{
	Events.resize(FrameCount);
}

void PhysSimulator::AddEvent(float Time, AAdvPhysEventBase* Event, float Interval, int FrameCount)
{
	const int FrameIndex = FPlatformMath::Min(Time / Interval, FrameCount - 1);
	auto NewNode = std::make_unique<FPhysEventNode>();
	NewNode->EventActor = Event;
	if (!Events[FrameIndex])
	{
		Events[FrameIndex] = std::move(NewNode);
		return;
	}
	NewNode->Next = std::move(Events[FrameIndex]);
	Events[FrameIndex] = std::move(NewNode);
}

void PhysSimulator::FreeEvents()
{
	Events.clear();
	Events.shrink_to_fit();
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
	
	for (const auto& Mesh : ConvexMeshes)
	{
		Mesh.second->release();
	}
	ConvexMeshes.clear();

	ObservedBodies.clear();
	
	CreateSceneInternal();
}

void PhysSimulator::AddStaticBody(UStaticMeshComponent* Comp, EShapeType Type)
{
	if (!bIsInitialized)
	{
		FMessageLog("PhysSimulator").Error(
			FText::FromString("AddStaticBody requires PhysSimulator to be initialized")
			);
		return;
	}
	
	PhysCompoundShape CompoundShape;
	GetShapeInternal(Comp, Type, CompoundShape);

	const PxVec3 PLoc = U2PVector(Comp->GetComponentLocation());
	const PxQuat PQuat = U2PQuat(Comp->GetComponentRotation().Quaternion());
	
	const PxTransform& PTransform = PxTransform(PLoc, PQuat);
	const auto PBody = Physics->createRigidStatic(PTransform);

	for (const auto& PShape : CompoundShape.Shapes)
	{
		PBody->attachShape(*PShape);
	}
	
	Scene->addActor(*PBody);
}

void PhysSimulator::AddDynamicBody(UStaticMeshComponent* Comp, bool bUseSimpleGeometry)
{
	if (!bIsInitialized)
	{
		FMessageLog("PhysSimulator").Error(
			FText::FromString("AddDynamicBody requires PhysSimulator to be initialized")
			);
		return;
	}
	
	PhysCompoundShape CompoundShape;
	GetShapeInternal(Comp, bUseSimpleGeometry ? Simple : Aggregate, CompoundShape);

	const PxVec3 PLoc = U2PVector(Comp->GetComponentLocation());
	const PxQuat PQuat = U2PQuat(Comp->GetComponentRotation().Quaternion());
	
	const PxTransform& PTransform = PxTransform(PLoc, PQuat);
	PxRigidDynamic* PBody = Physics->createRigidDynamic(PTransform);

	for (const auto& PShape : CompoundShape.Shapes)
	{
		PBody->attachShape(*PShape);
	}

	Comp->SetSimulatePhysics(true);
	PxRigidBodyExt::setMassAndUpdateInertia(*PBody, Comp->GetMass());
	Comp->SetSimulatePhysics(false);
	Scene->addActor(*PBody);
	ObservedBodies.push_back(PBody);
}

void PhysSimulator::StartRecord(
	FPhysRecordData* Destination,
	float RecordInterval,
	int FrameCount,
	float GravityZ
	)
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

	if (RecordData->bEnableSOD)
	{
		RecordData->ObjSOD.Empty();
		RecordData->ObjSOD.Reserve(FrameCount * ObservedBodies.size());
		RecordData->ObjSOD.AddZeroed(FrameCount * ObservedBodies.size());
	}
	
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

		HandleEventsInternal(i);
		
		Scene->simulate(RecordData->FrameInterval);
		Scene->fetchResults(true);
		for (int j = 0; j < ObservedBodies.size(); j++)
		{
			auto& Frame = RecordData->ObjLocRot[i * ObservedBodies.size() + j];
			const auto& Pose = ObservedBodies[j]->getGlobalPose();
			Frame.Location = P2UVector(Pose.p);
			Frame.Rotation = UE::Math::TRotator(P2UQuat(Pose.q));
		}

		if (RecordData->bEnableSOD)
		{
			for (int j = 0; j < ObservedBodies.size(); j++)
			{
				auto& Frame = RecordData->ObjSOD[i * ObservedBodies.size() + j];
				const auto Bounds = ObservedBodies[j]->getWorldBounds();
				AdvPhysHashHelper::GetHash(
					Bounds,
					RecordData->HashWorldCenter,
					RecordData->HashCellSize,
					Frame.StartHash,
					Frame.EndHash
					);
				Frame.Bounds = FBox(P2UVector(Bounds.minimum), P2UVector(Bounds.maximum));
			}
		}
		RecordData->Progress = static_cast<float>(i + 1) / RecordData->FrameCount;
	}
	RecordData->Finished = true;
	bIsRecording = false;
}

void PhysSimulator::HandleEventsInternal(int Frame)
{
	auto& EventCursor = Events[Frame];
	while (EventCursor)
	{
		EventCursor->EventActor->DoEventPhysX(ObservedBodies);
		EventCursor = EventCursor->Next;
	}
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
}

void PhysSimulator::GetShapeInternal(const UStaticMeshComponent* Comp, EShapeType Type, PhysCompoundShape& OutShape)
{
	const auto& UPhysMat = Comp->GetMaterial(0)->GetPhysicalMaterial();
	const auto PMaterial = Physics->createMaterial(
			UPhysMat->StaticFriction,
			UPhysMat->Friction,
			UPhysMat->Restitution
		);
	
	if (Type == Simple)
	{
		const auto& PGeom = GetSimpleGeometry(Comp);
		OutShape = PhysCompoundShape();
		OutShape.Shapes.push_back(Physics->createShape(*PGeom, *PMaterial));
		return;
	}

	const auto UScale = Comp->GetComponentScale();
	const auto PScale = U2PVector(UScale);
	
	if (Type == TriMesh)
	{
		auto& TriMeshes = Comp->GetStaticMesh()->GetBodySetup()->ChaosTriMeshes;
		for (auto& Mesh : TriMeshes)
		{
			PxTriangleMeshDesc MeshDesc;
			
			const auto& UVerts = Mesh->Particles().AllX();
			const auto& Indices = Mesh->Elements().GetSmallIndexBuffer();
			
			std::vector<PxVec3> PVerts;
			PVerts.reserve(UVerts.Num());
			for (const auto Vert : UVerts)
			{
				PVerts.push_back(PxVec3(Vert[0], Vert[1], Vert[2]));
			}

			std::vector<PxU32> PIndices;
			PIndices.reserve(Indices.Num() * 3);
			for (const auto Tri : Indices)
			{
				PIndices.push_back(Tri[0]);
				PIndices.push_back(Tri[1]);
				PIndices.push_back(Tri[2]);
			}
			
			MeshDesc.points.count = PVerts.size();
			MeshDesc.points.stride = sizeof(PxVec3);
			MeshDesc.points.data = PVerts.data();

			MeshDesc.triangles.count = PIndices.size() / 3;
			MeshDesc.triangles.stride = 3*sizeof(PxU32);
			MeshDesc.triangles.data = PIndices.data();

			const auto TriMesh = Cooking->createTriangleMesh(MeshDesc, Physics->getPhysicsInsertionCallback());
			PxTriangleMeshGeometry TriGeom;
			TriGeom.triangleMesh = TriMesh;
			TriGeom.scale = PxMeshScale(PScale);
			const auto NewShape = Physics->createShape(TriGeom, *PMaterial);
			OutShape.Shapes.push_back(NewShape);
		}
		return;
	}
	
	const auto& AggGeom = Comp->GetStaticMesh()->GetBodySetup()->AggGeom;
	const auto NumOfAgg = AggGeom.BoxElems.Num() +
		AggGeom.ConvexElems.Num() +
		AggGeom.SphereElems.Num() +
		AggGeom.SphylElems.Num() +
		AggGeom.TaperedCapsuleElems.Num();
	
	OutShape = PhysCompoundShape();
	OutShape.Shapes.reserve(NumOfAgg);
		
	for (auto& Box : AggGeom.BoxElems)
	{
		PxBoxGeometry PGeom(Box.X * UScale.X / 2.0f, Box.Y * UScale.Y / 2.0f, Box.Z * UScale.Z / 2.0f);
		PxTransform PTransform = U2PTransform(Box.GetTransform());
		PTransform.p.x *= UScale.X; // Translation in object space is affected by world transform
		PTransform.p.y *= UScale.Y;
		PTransform.p.z *= UScale.Z;
		
		const auto NewShape = Physics->createShape(PGeom, *PMaterial);
		NewShape->setLocalPose(PTransform);
		OutShape.Shapes.push_back(NewShape);
	}

	for (int i = 0; i < AggGeom.ConvexElems.Num(); i++)
	{
		const auto PMesh = GetConvexMeshInternal(Comp->GetStaticMesh(), i);
		if (PMesh == nullptr) continue;
		PxConvexMeshGeometry PGeom(PMesh, PxMeshScale(PScale));
		PxTransform PTransform = U2PTransform(AggGeom.ConvexElems[i].GetTransform());
		PTransform.p.x *= UScale.X;
		PTransform.p.y *= UScale.Y;
		PTransform.p.z *= UScale.Z;
		const auto NewShape = Physics->createShape(PGeom, *PMaterial);
		NewShape->setLocalPose(PTransform);
		OutShape.Shapes.push_back(NewShape);
	}

	for (auto& Sphere : AggGeom.SphereElems)
	{
		PxSphereGeometry PGeom(Sphere.Radius * UScale.X);
		PxTransform PTransform = U2PTransform(Sphere.GetTransform());
		PTransform.p.x *= UScale.X;
		PTransform.p.y *= UScale.Y;
		PTransform.p.z *= UScale.Z;
		
		const auto NewShape = Physics->createShape(PGeom, *PMaterial);
		NewShape->setLocalPose(PTransform);
		OutShape.Shapes.push_back(NewShape);
	}

	for (auto& Capsule : AggGeom.SphylElems)
	{
		// TODO scale capsules?
		PxCapsuleGeometry PGeom(Capsule.Radius, Capsule.Length / 2.0f);
		PxTransform PTransform = U2PTransform(Capsule.GetTransform());
		PTransform.p.x *= UScale.X;
		PTransform.p.y *= UScale.Y;
		PTransform.p.z *= UScale.Z;
		
		const auto NewShape = Physics->createShape(PGeom, *PMaterial);
		NewShape->setLocalPose(PTransform);
		OutShape.Shapes.push_back(NewShape);
	}

	for (auto& Cylinder : AggGeom.TaperedCapsuleElems)
	{
		// TODO
	}

	if (OutShape.Shapes.size() > 0) return;
	// Use render vertices for fallback collision convex mesh generation

	const auto PMesh = GetConvexMeshInternal(Comp->GetStaticMesh(), -1);
	if (PMesh == nullptr) return;
	PxConvexMeshGeometry PGeom(PMesh, PxMeshScale(PScale));
		
	const auto NewShape = Physics->createShape(PGeom, *PMaterial);
	OutShape.Shapes.push_back(NewShape);
}

std::shared_ptr<PxGeometry> PhysSimulator::GetSimpleGeometry(const UStaticMeshComponent* Comp) const
{
	// Use ColShape to generate simple geometry regardless of Collision Body used by component
	const auto UColShape = Comp->GetCollisionShape();
	
	switch (UColShape.ShapeType)
	{
	case ECollisionShape::Line:
		// Unsupported TODO?
		break;
	case ECollisionShape::Box:
		return std::make_shared<PxBoxGeometry>(U2PVector(UColShape.GetBox()));
	case ECollisionShape::Sphere:
		return std::make_shared<PxSphereGeometry>(UColShape.GetSphereRadius());
	case ECollisionShape::Capsule:
		return std::make_shared<PxCapsuleGeometry>(UColShape.GetCapsuleRadius(), UColShape.GetCapsuleHalfHeight());
	default: ;
	}
	FMessageLog("PhysSimulator").Error(
			FText::FromString("Unsupported Geometry Type")
		);
	return nullptr;
}

PxConvexMesh* PhysSimulator::GetConvexMeshInternal(UStaticMesh* Mesh, int ConvexElemIndex)
{
	const uint64 Id = Mesh->GetUniqueID() | (static_cast<uint64>(ConvexElemIndex) << 32);

	if (ConvexMeshes.count(Id))
		return ConvexMeshes[Id];

	std::vector<PxVec3> PVerts;
	
	if (ConvexElemIndex == -1)
	{
		const auto& VBuffer = Mesh->GetRenderData()->LODResources[0].VertexBuffers.PositionVertexBuffer;
		if (!VBuffer.GetAllowCPUAccess()) return nullptr;
		PVerts.reserve(VBuffer.GetNumVertices());
		for (unsigned i = 0u; i < VBuffer.GetNumVertices(); i++)
		{
			const auto Pos = VBuffer.VertexPosition(i);
			PVerts.push_back(PxVec3(Pos.X, Pos.Y, Pos.Z));
		}
	}
	else
	{
		const auto& Convex = Mesh->GetBodySetup()->AggGeom.ConvexElems[ConvexElemIndex];

		TSet<int> UsedIndices;
		for (int i = 0; i < Convex.IndexData.Num(); i++)
		{
			UsedIndices.Add(Convex.IndexData[i]);
		}
	
		PVerts.resize(UsedIndices.Num());
		int Cursor = 0;
		for (const int Index : UsedIndices)
		{
			PVerts[Cursor] = U2PVector(Convex.VertexData[Index]);
			Cursor++;
		}
	}
	
	PxConvexMeshDesc convexDesc;
	convexDesc.points.count     = PVerts.size();
	convexDesc.points.stride    = sizeof(PxVec3);
	convexDesc.points.data      = PVerts.data();
	convexDesc.flags            = PxConvexFlag::eCOMPUTE_CONVEX |
		PxConvexFlag::eCHECK_ZERO_AREA_TRIANGLES |
			PxConvexFlag::eQUANTIZE_INPUT |
				PxConvexFlag::eDISABLE_MESH_VALIDATION | 
					PxConvexFlag::eFAST_INERTIA_COMPUTATION;
	convexDesc.vertexLimit		= 40;
	
	const auto ConvexMesh = Cooking->createConvexMesh(convexDesc,
		Physics->getPhysicsInsertionCallback());
	if (ConvexMesh == nullptr)
		return nullptr;
	ConvexMeshes[Id] = ConvexMesh;
	return ConvexMesh;
}
