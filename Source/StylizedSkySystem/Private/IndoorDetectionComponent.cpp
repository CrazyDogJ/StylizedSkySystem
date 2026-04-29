#include "IndoorDetectionComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"

// ─────────────────────────────────────────────────────────────────────────────
//  Fibonacci sphere: evenly distributes N directions over the unit sphere.
//  Much better coverage than pure random, and deterministic.
// ─────────────────────────────────────────────────────────────────────────────
static TArray<FVector> GenerateFibonacciSphere(int32 N)
{
	TArray<FVector> Dirs;
	Dirs.Reserve(N);

	const float GoldenAngle = PI * (3.f - FMath::Sqrt(5.f)); // ~2.399 rad

	for (int32 i = 0; i < N; ++i)
	{
		const float Y     = 1.f - (i / FMath::Max(1.f, float(N - 1))) * 2.f; // [-1, 1]
		const float Radius = FMath::Sqrt(FMath::Max(0.f, 1.f - Y * Y));
		const float Theta  = GoldenAngle * i;

		Dirs.Add(FVector(
			FMath::Cos(Theta) * Radius,
			FMath::Sin(Theta) * Radius,
			Y
		).GetSafeNormal());
	}
	return Dirs;
}

// ─────────────────────────────────────────────────────────────────────────────
UIndoorDetectionComponent::UIndoorDetectionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// Default: collide with static world geometry only
	TraceObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_WorldStatic));
}

// ─────────────────────────────────────────────────────────────────────────────
void UIndoorDetectionComponent::BeginPlay()
{
	Super::BeginPlay();
	RebuildDirectionCache();
	// Kick off the first scan immediately so we have a result right away
	FireAsyncBatch();
}

// ─────────────────────────────────────────────────────────────────────────────
void UIndoorDetectionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	// Nothing async-specific to clean up; UE handles pending trace delegates
}

// ─────────────────────────────────────────────────────────────────────────────
void UIndoorDetectionComponent::TickComponent(float DeltaTime,
                                              ELevelTick TickType,
                                              FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ── 1. Smoothly interpolate IndoorAlpha toward TargetAlpha ───────────────
	const float PrevAlpha = IndoorAlpha;
	IndoorAlpha = FMath::FInterpTo(IndoorAlpha, TargetAlpha, DeltaTime, AlphaSmoothSpeed);

	if (!FMath::IsNearlyEqual(IndoorAlpha, PrevAlpha, 0.001f))
	{
		OnIndoorAlphaChanged.Broadcast(IndoorAlpha);
	}

	// ── 2. Fire the next async scan when the timer expires ──────────────────
	ScanTimer -= DeltaTime;
	if (ScanTimer <= 0.f)
	{
		ScanTimer = ScanInterval;

		// Rebuild direction cache only when NumTraces changed at runtime
		if (NumTraces != CachedDirectionCount)
		{
			RebuildDirectionCache();
		}

		FireAsyncBatch();
	}
}

// ─────────────────────────────────────────────────────────────────────────────
void UIndoorDetectionComponent::RequestScanNow()
{
	ScanTimer = ScanInterval; // reset the auto-timer too
	FireAsyncBatch();
}

// ─────────────────────────────────────────────────────────────────────────────
void UIndoorDetectionComponent::RebuildDirectionCache()
{
	CachedDirections     = GenerateFibonacciSphere(NumTraces);
	CachedDirectionCount = NumTraces;
}

// ─────────────────────────────────────────────────────────────────────────────
FCollisionQueryParams UIndoorDetectionComponent::BuildQueryParams() const
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(IndoorDetection), false);
	// Ignore the owning actor so we don't hit our own capsule / mesh
	Params.AddIgnoredActor(GetOwner());
	return Params;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Fire all rays asynchronously in a single frame.
//  Each callback decrements PendingResponses; the last one calls FinaliseRatio.
// ─────────────────────────────────────────────────────────────────────────────
void UIndoorDetectionComponent::FireAsyncBatch()
{
	UWorld* World = GetWorld();
	if (!World || CachedDirections.IsEmpty()) return;

	// Guard: don't fire a new batch while the previous one is still in flight.
	// This keeps the hit counter coherent without any mutex.
	if (PendingResponses > 0) return;

	FVector Origin;
	if (PrimitiveComponent)
	{
		Origin = PrimitiveComponent->GetComponentLocation();
	}
	else
	{
		Origin = GetOwner()->GetActorLocation();
	}
	FCollisionQueryParams QParams = BuildQueryParams();
	QParams.bIgnoreTouches = true;

	PendingHits      = 0;
	PendingResponses = CachedDirections.Num();
	BatchSize        = CachedDirections.Num();

	for (const FVector& Dir : CachedDirections)
	{
		const FVector End = Origin + Dir * TraceLength;

		auto TraceDelegate = FTraceDelegate::CreateUObject(this, &UIndoorDetectionComponent::OnSingleTraceComplete);
		
		// Async trace – result delivered via delegate on the game thread
		World->AsyncLineTraceByObjectType(
			EAsyncTraceType::Single,
			Origin,
			End,
			TraceObjectTypes,
			QParams,
			&TraceDelegate
		);

#if ENABLE_DRAW_DEBUG
		// Uncomment to visualise rays in the editor (disable in shipping!)
		// DrawDebugLine(World, Origin, End, FColor::Cyan, false, ScanInterval * 0.9f, 0, 1.f);
#endif
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  Called on the game thread by the async trace system for each completed ray.
// ─────────────────────────────────────────────────────────────────────────────
void UIndoorDetectionComponent::OnSingleTraceComplete(const FTraceHandle& TraceHandle, FTraceDatum& TraceDatum)
{
	// Count blocking hits for this ray
	if (!TraceDatum.OutHits.IsEmpty() && TraceDatum.OutHits[0].bBlockingHit)
	{
		++PendingHits;
	}

	// When all rays in the batch have reported back, finalise the ratio
	if (--PendingResponses <= 0)
	{
		FinaliseRatio();
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  Converts the raw hit count into a normalised ratio and updates TargetAlpha.
//
//  You can apply any remapping curve here – a simple linear ratio works well,
//  but a smoothstep gives a more natural crossfade around the threshold.
// ─────────────────────────────────────────────────────────────────────────────
void UIndoorDetectionComponent::FinaliseRatio()
{
	if (BatchSize <= 0) return;

	// Raw ratio: fraction of rays that were blocked
	RawHitRatio = float(PendingHits) / float(BatchSize);

	// ── Optional: smoothstep remap for a softer threshold ───────────────────
	//   Tweak InRangeLow / InRangeHigh to define the "transition band".
	//   e.g. 0.35 → 0.75 means:
	//     < 35 % hits  →  alpha = 0  (outdoor)
	//     > 75 % hits  →  alpha = 1  (indoor)
	const float InRangeLow  = 0.35f;
	const float InRangeHigh = 0.75f;

	const float T = FMath::Clamp(
		(RawHitRatio - InRangeLow) / FMath::Max(InRangeHigh - InRangeLow, KINDA_SMALL_NUMBER),
		0.f, 1.f
	);

	// Smoothstep: 3t²−2t³
	TargetAlpha = T * T * (3.f - 2.f * T);

	// ── Linear alternative (uncomment to use) ───────────────────────────────
	// TargetAlpha = RawHitRatio;
}
