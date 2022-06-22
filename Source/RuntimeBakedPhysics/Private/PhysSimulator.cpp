// Fill out your copyright notice in the Description page of Project Settings.
#include "PhysSimulator.h"
#include "AdvPhysScene.h"
#include "PtouConversions.h"

#include "PhysXPublicCore.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

PhysSimulator::PhysSimulator(): RecordData(nullptr), Foundation(nullptr), Physics(nullptr), Dispatcher(nullptr),
                                Scene(nullptr),
                                Pvd(nullptr), Cooking(nullptr),
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
	PxPvdTransport* Transport = PxDefaultPvdSocketTransportCreate("localhost", 5425, 10);
	Pvd->connect(*Transport,PxPvdInstrumentationFlag::eALL);

	Physics = PxCreatePhysics(PX_PHYSICS_VERSION, *Foundation, PxTolerancesScale(), true, Pvd);
	
	CreateSceneInternal();

	Cooking = PxCreateCooking(PX_PHYSICS_VERSION, *Foundation, PxCookingParams(Physics->getTolerancesScale()));
	
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
	Cooking->release();
	Foundation->release();
	
	bIsInitialized = false;
}

void PhysSimulator::ReserveEvents(int FrameCount)
{
	Events.resize(FrameCount);
}

void PhysSimulator::AddEvents(TArray<std::tuple<float, std::any>>& Pairs, float Interval, int FrameCount)
{
	for (const auto& P : Pairs)
	{
		const float Time = std::get<0>(P);
		AddEvent(Time, std::get<1>(P), Interval, FrameCount);
	}
}

void PhysSimulator::AddEvent(float Time, std::any Event, float Interval, int FrameCount) // Unnecessary copy of arg 'Event'?
{
	const int FrameIndex = FPlatformMath::Min(Time / Interval, FrameCount - 1);
	auto NewNode = std::make_unique<FPhysEventNode>();
	NewNode->Event = Event;
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

void PhysSimulator::AddStaticBody(UStaticMeshComponent* Comp, bool bUseSimpleGeometry)
{
	if (!bIsInitialized)
	{
		FMessageLog("PhysSimulator").Error(
			FText::FromString("AddStaticBody requires PhysSimulator to be initialized")
			);
		return;
	}
	
	PhysCompoundShape CompoundShape;
	GetShapeInternal(Comp, bUseSimpleGeometry, CompoundShape);

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
	GetShapeInternal(Comp, bUseSimpleGeometry, CompoundShape);

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
		if (EventCursor->Event.type() == typeid(FPhysEvent_Explosion))
			HandleExplosionEventInternal(std::any_cast<FPhysEvent_Explosion>(EventCursor->Event));
		
		EventCursor = EventCursor->Next;
	}
}

void PhysSimulator::HandleExplosionEventInternal(FPhysEvent_Explosion&& Explosion)
{
	const auto ExplosionPos = U2PVector(Explosion.Position);

	// Fall-Offs
	const float MaxSqr = Explosion.FallOffMaxDistance * Explosion.FallOffMaxDistance;
	const float MinSqr = Explosion.FallOffMinDistance * Explosion.FallOffMinDistance;
	const float MinMaxDiff = Explosion.FallOffMaxDistance - Explosion.FallOffMinDistance;
			
	for (const auto& Body : ObservedBodies)
	{
		PxVec3 Dir = Body->getGlobalPose().p - ExplosionPos;
		float SqrDist = Dir.magnitudeSquared();
		Dir.normalize();
		float Multiplier = Explosion.Impulse;
		if (SqrDist > MaxSqr)
			continue;
		if (SqrDist > MinSqr)
			Multiplier *= 1.0f - (FPlatformMath::Sqrt(SqrDist) - Explosion.FallOffMinDistance) / MinMaxDiff;
				
		PxVec3 Impulse = Dir * Multiplier;
		PxRigidBodyExt::addForceAtPos(*Body, Impulse, ExplosionPos, PxForceMode::eIMPULSE);
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

void PhysSimulator::GetShapeInternal(const UStaticMeshComponent* Comp, bool bUseSimpleGeometry, PhysCompoundShape& OutShape)
{
	const auto& UPhysMat = Comp->GetMaterial(0)->GetPhysicalMaterial();
	const auto PMaterial = Physics->createMaterial(
			UPhysMat->StaticFriction,
			UPhysMat->Friction,
			UPhysMat->Restitution
		);
	
	if (bUseSimpleGeometry)
	{
		const auto& PGeom = GetSimpleGeometry(Comp);
		OutShape = PhysCompoundShape();
		OutShape.Shapes.push_back(Physics->createShape(*PGeom, *PMaterial));
		return;
	}
	
	const auto UScale = Comp->GetComponentScale();
	const auto PScale = U2PVector(UScale);
	const auto& AggGeom = Comp->GetStaticMesh()->GetBodySetup()->AggGeom;
		
	OutShape = PhysCompoundShape();
	OutShape.Shapes.reserve(
		AggGeom.BoxElems.Num() +
		AggGeom.ConvexElems.Num() +
		AggGeom.SphereElems.Num() +
		AggGeom.SphylElems.Num() +
		AggGeom.TaperedCapsuleElems.Num());
		
	for (auto& Box : AggGeom.BoxElems)
	{
		// TODO proper scaling to cubes?
		PxBoxGeometry PGeom(Box.X * UScale.X / 2.0f, Box.Y * UScale.Y / 2.0f, Box.Z * UScale.Z / 2.0f);
		PxTransform PTransform = U2PTransform(Box.GetTransform());
		const auto NewShape = Physics->createShape(PGeom, *PMaterial);
		NewShape->setLocalPose(PTransform);
		OutShape.Shapes.push_back(NewShape);
	}

	for (int i = 0; i < AggGeom.ConvexElems.Num(); i++)
	{
		const auto PMesh = GetConvexMeshInternal(Comp->GetStaticMesh(), i);
		PxConvexMeshGeometry PGeom(PMesh, PxMeshScale(PScale));
		PxTransform PTransform = U2PTransform(AggGeom.ConvexElems[i].GetTransform());
		const auto NewShape = Physics->createShape(PGeom, *PMaterial);
		NewShape->setLocalPose(PTransform);
		OutShape.Shapes.push_back(NewShape);
	}

	for (auto& Sphere : AggGeom.SphereElems)
	{
		PxSphereGeometry PGeom(Sphere.Radius * UScale.X);
		PxTransform PTransform = U2PTransform(Sphere.GetTransform());
		const auto NewShape = Physics->createShape(PGeom, *PMaterial);
		NewShape->setLocalPose(PTransform);
		OutShape.Shapes.push_back(NewShape);
	}

	for (auto& Capsule : AggGeom.SphylElems)
	{
		// TODO scale capsules?
		PxCapsuleGeometry PGeom(Capsule.Radius, Capsule.Length / 2.0f);
		PxTransform PTransform = U2PTransform(Capsule.GetTransform());
		const auto NewShape = Physics->createShape(PGeom, *PMaterial);
		NewShape->setLocalPose(PTransform);
		OutShape.Shapes.push_back(NewShape);
	}

	for (auto& Cylinder : AggGeom.TaperedCapsuleElems)
	{
		// TODO
	}
}

/**
 * @brief You must free the returned pointer to PxGeometry!
 * @param Comp 
 * @return 
 */
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
	
	const auto& Convex = Mesh->GetBodySetup()->AggGeom.ConvexElems[ConvexElemIndex];

	TSet<int> UsedIndices;
	for (int i = 0; i < Convex.IndexData.Num(); i++)
	{
		UsedIndices.Add(Convex.IndexData[i]);
	}
	
	std::vector<PxVec3> PVerts;
	
	PVerts.resize(UsedIndices.Num());
	int Cursor = 0;
	for (const int Index : UsedIndices)
	{
		PVerts[Cursor] = U2PVector(Convex.VertexData[Index]);
		Cursor++;
	}
			
	PxConvexMeshDesc convexDesc;
	convexDesc.points.count     = PVerts.size();
	convexDesc.points.stride    = sizeof(PxVec3);
	convexDesc.points.data      = PVerts.data();
	convexDesc.flags            = PxConvexFlag::eCOMPUTE_CONVEX | PxConvexFlag::eCHECK_ZERO_AREA_TRIANGLES | PxConvexFlag::eQUANTIZE_INPUT;
	convexDesc.vertexLimit		= 40;
			
	// mesh should be validated before cooking without the mesh cleaning
	// Remove on release build?
	bool res = Cooking->validateConvexMesh(convexDesc);
	PX_ASSERT(res);

	const auto ConvexMesh = Cooking->createConvexMesh(convexDesc,
		Physics->getPhysicsInsertionCallback());
	ConvexMeshes[Id] = ConvexMesh;
	return ConvexMesh;
}
