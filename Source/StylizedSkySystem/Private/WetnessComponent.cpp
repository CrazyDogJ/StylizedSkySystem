// Fill out your copyright notice in the Description page of Project Settings.

#include "WetnessComponent.h"

#include "Components/MeshComponent.h"
#include "Engine/World.h"
#include "StylizedSkySubsystem.h"
#include "StylizedSkyActor.h"
#include "StylizedSkyWeatherData.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UWetnessComponent::UWetnessComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

TObjectPtr<UStylizedSkySubsystem> UWetnessComponent::GetStylizedSkySubsystem() const
{
	return GetWorld()->GetSubsystem<UStylizedSkySubsystem>();
}

TObjectPtr<AStylizedSkyActor> UWetnessComponent::GetStylizedSkyActor() const
{
	if (const auto Sub = GetStylizedSkySubsystem())
	{
		return Sub->TryGetSkyActor();
	}

	return nullptr;
}

FGameplayTag UWetnessComponent::GetCurrentWeatherTag() const
{
	if (const auto Sky = GetStylizedSkyActor())
	{
		if (Sky->WeatherPreset)
		{
			return Sky->WeatherPreset->WeatherTag;
		}
	}

	return FGameplayTag::EmptyTag;
}

bool UWetnessComponent::IsWeatherWet() const
{
	const auto Tag = GetCurrentWeatherTag();
	if (Tag.IsValid())
	{
		if (WetnessTags.HasTag(Tag))
		{
			return true;
		}
	}
	
	return false;
}

bool UWetnessComponent::IsOwnerSwimming() const
{
	if (CharacterMovementComponent)
	{
		return CharacterMovementComponent->IsSwimming();
	}

	return false;
}

void UWetnessComponent::CeilingTrace()
{
	const auto World = GetWorld();
	if (!World)
	{
		return;
	}

	const auto Origin = GetOwner()->GetActorLocation();
	const auto End = Origin + FVector(0, 0, WetnessTraceDistance);
	
	FCollisionQueryParams QParams(SCENE_QUERY_STAT(WetnessComponent), false);
	QParams.AddIgnoredActor(GetOwner());
	QParams.bIgnoreTouches = true;

	auto TraceDelegate = FTraceDelegate::CreateUObject(this, &UWetnessComponent::OnSingleTraceComplete);
	
	World->AsyncLineTraceByObjectType(
			EAsyncTraceType::Single,
			Origin,
			End,
			WetnessTraceObjectTypes,
			QParams,
			&TraceDelegate
		);
}

void UWetnessComponent::OnSingleTraceComplete(const FTraceHandle& TraceHandle, FTraceDatum& TraceDatum)
{
	if (!TraceDatum.OutHits.IsEmpty() && TraceDatum.OutHits[0].bBlockingHit)
	{
		bHasCeiling = true;
	}
	else
	{
		bHasCeiling = false;
	}
}

void UWetnessComponent::UpdateWetness(float DeltaTime)
{
	if (IsOwnerSwimming())
	{
		Wetness = FMath::Clamp(Wetness + DeltaTime * SwimWetSpeed, 0.0f, 1.0f);
	}
	else if (IsWeatherWet() && !bHasCeiling)
	{
		Wetness = FMath::Clamp(Wetness + DeltaTime * WetSpeed, 0.0f, 1.0f);
	}
	else
	{
		Wetness = FMath::Clamp(Wetness - DeltaTime * DrySpeed, 0.0f, 1.0f);
	}
}

void UWetnessComponent::UpdateMeshesParams(const float InWetness) const
{
	const auto Owner = GetOwner();
	Owner->ForEachComponent<UMeshComponent>(true, [&, InWetness](UMeshComponent* Component)
	{
		Component->SetScalarParameterValueOnMaterials(WetnessName, InWetness);
	});
}

void UWetnessComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter)
	{
		CharacterMovementComponent = OwnerCharacter->GetCharacterMovement();
	}
}

void UWetnessComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                      FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	CeilingTrace();
	UpdateWetness(DeltaTime);
	UpdateMeshesParams(Wetness);
}

