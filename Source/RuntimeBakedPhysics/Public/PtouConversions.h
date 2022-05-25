#pragma once
#include <PxPlane.h>
#include <PxQuat.h>
#include <PxVec3.h>
#include <PxVec4.h>

using namespace physx;

FORCEINLINE_DEBUGGABLE PxVec3 U2PVector(const FVector& UVec)
{
	return PxVec3(UVec.X, UVec.Y, UVec.Z);
}

FORCEINLINE_DEBUGGABLE PxVec4 U2PVector(const FVector4& UVec)
{
	return PxVec4(UVec.X, UVec.Y, UVec.Z, UVec.W);
}

/** Convert Unreal FQuat to PhysX PxQuat */
FORCEINLINE_DEBUGGABLE PxQuat U2PQuat(const FQuat& UQuat)
{
	return PxQuat( UQuat.X, UQuat.Y, UQuat.Z, UQuat.W );
}

/** Convert Unreal FPlane to PhysX plane def */
FORCEINLINE_DEBUGGABLE PxPlane U2PPlane(const FPlane& Plane)
{
	return PxPlane(Plane.X, Plane.Y, Plane.Z, -Plane.W);
}

/** Convert PhysX PxVec3 to Unreal FVector */
FORCEINLINE_DEBUGGABLE FVector P2UVector(const PxVec3& PVec)
{
	return FVector(PVec.x, PVec.y, PVec.z);
}

FORCEINLINE_DEBUGGABLE FVector4 P2UVector(const PxVec4& PVec)
{
	return FVector4(PVec.x, PVec.y, PVec.z, PVec.w);
}

/** Convert PhysX PxQuat to Unreal FQuat */
FORCEINLINE_DEBUGGABLE FQuat P2UQuat(const PxQuat& PQuat)
{
	return FQuat(PQuat.x, PQuat.y, PQuat.z, PQuat.w);
}

/** Convert PhysX plane def to Unreal FPlane */
FORCEINLINE_DEBUGGABLE FPlane P2UPlane(const PxReal P[4])
{
	return FPlane(P[0], P[1], P[2], -P[3]);
}

FORCEINLINE_DEBUGGABLE FPlane P2UPlane(const PxPlane& Plane)
{
	return FPlane(Plane.n.x, Plane.n.y, Plane.n.z, -Plane.d);
}

/** Convert PhysX Barycentric Vec3 to FVector4 */
FORCEINLINE_DEBUGGABLE FVector4 P2U4BaryCoord(const PxVec3& PVec)
{
	return FVector4(PVec.x, PVec.y, 1.f - PVec.x - PVec.y, PVec.z);
}