#include "CoreMinimal.h"
#include "AdvPhysScene.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "RecordScene.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDelayOutputPin);

/**
 * 
 */
UCLASS()
class URecordScene : public UBlueprintAsyncActionBase
{
	GENERATED_UCLASS_BODY()

public:

	UPROPERTY(BlueprintAssignable)
	FDelayOutputPin Completed;
	
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
	static URecordScene* RecordScene(AAdvPhysScene* Scene, const float Interval, const int FrameCount);

	virtual void Activate() override;

private:
	FDelegateHandle Handle;
	AAdvPhysScene* Scene;
	float Interval;
	float FrameCount;
	
	UFUNCTION()
	void RecordFinished();
};