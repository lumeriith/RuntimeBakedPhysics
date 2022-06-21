#include "RecordScene.h"
#include "Engine/World.h"
#include "TimerManager.h"

URecordScene::URecordScene(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer), Scene(nullptr), Interval(0), FrameCount(0)
{
}

URecordScene* URecordScene::RecordScene(AAdvPhysScene* Scene, const float Interval, const int FrameCount)
{
	URecordScene* Node = NewObject<URecordScene>();
	Node->Scene = Scene;
	Node->Interval = Interval;
	Node->FrameCount = FrameCount;
	return Node;
}

void URecordScene::Activate()
{
	if (!Scene)
	{
		return;
	}
	Handle = Scene->RecordFinished.AddUFunction(this, "RecordFinished");
	Scene->Record(Interval, FrameCount);
}

void URecordScene::RecordFinished()
{
	Completed.Broadcast();
	Scene->RecordFinished.Remove(Handle);
}