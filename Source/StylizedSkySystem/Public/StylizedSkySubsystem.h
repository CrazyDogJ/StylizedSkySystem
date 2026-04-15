// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "StylizedSkySubsystem.generated.h"

class AStylizedSkyActor;

UCLASS()
class STYLIZEDSKYSYSTEM_API UStylizedSkySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

	virtual void Deinitialize() override;
	
public:
	UPROPERTY()
	AStylizedSkyActor* SkyActor;

	UFUNCTION(BlueprintCallable)
	AStylizedSkyActor* TryGetSkyActor();
	
	UFUNCTION(BlueprintPure)
	bool GetRawTime(float& OutTime) const;
	
	UFUNCTION(BlueprintPure)
	bool GetDateTime(FDateTime& OutDateTime) const;
};
