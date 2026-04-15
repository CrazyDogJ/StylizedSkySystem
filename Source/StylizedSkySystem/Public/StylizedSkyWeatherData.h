// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "Rendering/ExponentialHeightFogData.h"
#include "StylizedSkyWeatherData.generated.h"

class UNiagaraSystem;

UCLASS(BlueprintType)
class STYLIZEDSKYSYSTEM_API UStylizedSkyWeatherData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Basic")
	FGameplayTag WeatherTag;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Cloud")
	UMaterialInstance* WeatherCloudMaterialInstance = nullptr;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Fog")
	FExponentialHeightFogData SecondFogData;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Fog")
	FLinearColor FogInscatteringLuminance;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Light")
	float LightMultiplier = 1.0f;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Light")
	FLinearColor LightColorMultiplier = FLinearColor::White;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Niagara")
	UNiagaraSystem* WeatherNiagaraSystem = nullptr;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Niagara")
	TMap<FString, float> WeatherNiagaraOverrideFloat;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Niagara")
	TMap<FString, int> WeatherNiagaraOverrideInt;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Niagara")
	TMap<FString, FVector> WeatherNiagaraOverrideVector;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Sound")
	USoundBase* WeatherSound = nullptr;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Time")
	float FadeTime = 5.0f;

	float GetFadeAlphaSpeed() const { return 1 / FadeTime; }
};
