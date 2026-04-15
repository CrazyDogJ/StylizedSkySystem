// Fill out your copyright notice in the Description page of Project Settings.


#include "StylizedSkySubsystem.h"

#include "StylizedSkyActor.h"
#include "Kismet/GameplayStatics.h"

void UStylizedSkySubsystem::Deinitialize()
{
	Super::Deinitialize();

	SkyActor = nullptr;
}

AStylizedSkyActor* UStylizedSkySubsystem::TryGetSkyActor()
{
	if (SkyActor)
	{
		return SkyActor;
	}

	const auto FoundActor = UGameplayStatics::GetActorOfClass(this, AStylizedSkyActor::StaticClass());
	if (const auto CastActor = Cast<AStylizedSkyActor>(FoundActor))
	{
		SkyActor = CastActor;
		return SkyActor;
	}

	return nullptr;
}

bool UStylizedSkySubsystem::GetRawTime(float& OutTime) const
{
	if (SkyActor)
	{
		OutTime = SkyActor->RawDayTime;
		return true;
	}

	return false;
}

bool UStylizedSkySubsystem::GetDateTime(FDateTime& OutDateTime) const
{
	if (SkyActor)
	{
		OutDateTime = SkyActor->GetDateTime();
		return true;
	}

	return false;
}
