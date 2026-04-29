#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "IndoorDetectionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIndoorAlphaChanged, float, NewAlpha);

/**
 * UIndoorDetectionComponent
 *
 * Async multi-directional line trace component that estimates whether the owning
 * character is inside an enclosed space.  A hit-ratio across N sample directions
 * is converted to a smooth [0..1] alpha you can drive weather audio / post-process
 * volumes with.
 *
 *  0.0  = fully outdoors   (few or no traces hit geometry overhead / around)
 *  1.0  = fully indoors    (most traces blocked by surrounding geometry)
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class STYLIZEDSKYSYSTEM_API UIndoorDetectionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UIndoorDetectionComponent();

	// ── Tuning ─────────────────────────────────────────────────────────────

	/** How many rays to cast per scan. Higher = more accurate, more expensive. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Indoor Detection", meta = (ClampMin = "4", ClampMax = "128"))
	int32 NumTraces = 32;

	/** Max length of each ray (cm). Should comfortably exceed your largest indoor room. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Indoor Detection", meta = (ClampMin = "100.0"))
	float TraceLength = 2000.f;

	/** Seconds between successive scans. Keep >= 0.1 to stay async-friendly. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Indoor Detection", meta = (ClampMin = "0.05"))
	float ScanInterval = 0.3f;

	/**
	 * Smoothing speed for the alpha interpolation.
	 * Higher = snappier transition, lower = slower fade.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Indoor Detection", meta = (ClampMin = "0.1"))
	float AlphaSmoothSpeed = 3.f;

	/**
	 * Object types the traces should collide with.
	 * Typically WorldStatic is enough for architectural geometry.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Indoor Detection")
	TArray<TEnumAsByte<EObjectTypeQuery>> TraceObjectTypes;

	// ── Output ─────────────────────────────────────────────────────────────

	/** Smoothed alpha: 0 = outdoor, 1 = indoor. Read this to drive audio / VFX. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Indoor Detection")
	float IndoorAlpha = 0.f;

	/** Fires every tick while IndoorAlpha is changing (threshold 0.001). */
	UPROPERTY(BlueprintAssignable, Category = "Indoor Detection")
	FOnIndoorAlphaChanged OnIndoorAlphaChanged;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	USceneComponent* PrimitiveComponent = nullptr;
	
	// ── Blueprint helpers ───────────────────────────────────────────────────

	/** Manually trigger a scan immediately (async, result arrives next frame). */
	UFUNCTION(BlueprintCallable, Category = "Indoor Detection")
	void RequestScanNow();

	/** Return the current raw (un-smoothed) hit ratio [0..1]. */
	UFUNCTION(BlueprintPure, Category = "Indoor Detection")
	float GetRawHitRatio() const { return RawHitRatio; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

private:
	// ── Internal state ──────────────────────────────────────────────────────

	float RawHitRatio      = 0.f;   // latest ratio from completed async batch
	float TargetAlpha      = 0.f;   // target we smoothly approach
	float ScanTimer        = 0.f;   // countdown to next scan
	int32 PendingHits      = 0;     // hits accumulated in current async batch
	int32 PendingResponses = 0;     // traces still awaiting callback
	int32 BatchSize        = 0;     // total traces fired this batch

	FCollisionQueryParams BuildQueryParams() const;
	void OnSingleTraceComplete(const FTraceHandle& TraceHandle, FTraceDatum& TraceDatum);
	void FireAsyncBatch();
	void FinaliseRatio();

	// Sphere distribution cache – rebuilt when NumTraces changes
	TArray<FVector> CachedDirections;
	int32           CachedDirectionCount = 0;
	void RebuildDirectionCache();
};
