// Fill out your copyright notice in the Description page of Project Settings.


#include "AdvPhysEvent_Explosion.h"

#include <PxRigidBodyExt.h>

#include "PtouConversions.h"

AAdvPhysEvent_Explosion::AAdvPhysEvent_Explosion() :
	Impulse(5000),
	FallOffMinDistance(10),
	FallOffMaxDistance(100)
{ }

void AAdvPhysEvent_Explosion::DoEventPhysX(std::vector<PxRigidDynamic*>& Bodies)
{
	Super::DoEventPhysX(Bodies);
	const auto ExplosionPos = U2PVector(GetActorLocation());

	// Fall-Offs
	const float MaxSqr = FallOffMaxDistance * FallOffMaxDistance;
	const float MinSqr = FallOffMinDistance * FallOffMinDistance;
	const float MinMaxDiff = FallOffMaxDistance - FallOffMinDistance;
			
	for (const auto& Body : Bodies)
	{
		PxVec3 Dir = Body->getGlobalPose().p - ExplosionPos;
		const float SqrDist = Dir.magnitudeSquared();
		Dir.normalize();
		float Multiplier = Impulse;
		if (SqrDist > MaxSqr)
			continue;
		if (SqrDist > MinSqr)
			Multiplier *= 1.0f - (FPlatformMath::Sqrt(SqrDist) - FallOffMinDistance) / MinMaxDiff;
				
		PxVec3 ImpulseVec = Dir * Multiplier;
		PxRigidBodyExt::addForceAtPos(*Body, ImpulseVec, ExplosionPos, PxForceMode::eIMPULSE);
	}
}

void AAdvPhysEvent_Explosion::DoEventUE(TArray<FPhysObject>& Dynamics)
{
	Super::DoEventUE(Dynamics);
	const auto ExplosionPos = GetActorLocation();

	// Fall-Offs
	const float MaxSqr = FallOffMaxDistance * FallOffMaxDistance;
	const float MinSqr = FallOffMinDistance * FallOffMinDistance;
	const float MinMaxDiff = FallOffMaxDistance - FallOffMinDistance;
			
	for (const auto& Obj : Dynamics)
	{
		const auto Dir = (Obj.Comp->GetComponentLocation() - ExplosionPos).GetSafeNormal();
		const float SqrDist = Dir.SizeSquared();
		
		float Multiplier = Impulse;
		if (SqrDist > MaxSqr)
			continue;
		if (SqrDist > MinSqr)
			Multiplier *= 1.0f - (FPlatformMath::Sqrt(SqrDist) - FallOffMinDistance) / MinMaxDiff;
				
		const auto ImpulseVec = Dir * Multiplier;
		Obj.Comp->AddImpulseAtLocation(ImpulseVec, ExplosionPos);
	}
}

