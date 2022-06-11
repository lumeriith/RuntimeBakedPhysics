// Fill out your copyright notice in the Description page of Project Settings.


#include "AdvPhysEvent_Explosion.h"

AAdvPhysEvent_Explosion::AAdvPhysEvent_Explosion() :
	Impulse(5000),
	FallOffMinDistance(10),
	FallOffMaxDistance(100)
{ }

std::any AAdvPhysEvent_Explosion::GetEvent()
{
	FPhysEvent_Explosion Event;
	Event.Impulse = Impulse;
	Event.FallOffMinDistance = FallOffMinDistance;
	Event.FallOffMaxDistance = FallOffMaxDistance;
	Event.Position = GetActorLocation();
	return Event;
}

