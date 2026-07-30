// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/EngineTypes.h"
#include "Components/ActorComponent.h"
#include "WetnessComponent.generated.h"

class AStylizedSkyActor;
class UStylizedSkySubsystem;
class UMeshComponent;
class UCharacterMovementComponent;
class UCapsuleComponent;
class ACharacter;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class STYLIZEDSKYSYSTEM_API UWetnessComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWetnessComponent();

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Wetness")
	ACharacter* OwnerCharacter = nullptr;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Wetness")
	UCharacterMovementComponent* CharacterMovementComponent = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Wetness")
	FGameplayTagContainer WetnessTags;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Wetness")
	float WetnessTraceDistance = 10000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wetness")
	TArray<TEnumAsByte<EObjectTypeQuery>> WetnessTraceObjectTypes;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Wetness")
	FName WetnessName = "Wetness";

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Wetness")
	float Wetness = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Wetness")
	float SwimWetSpeed = 0.4f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Wetness")
	float WetSpeed = 0.2f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Wetness")
	float DrySpeed = 0.1f;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Wetness")
	bool bHasCeiling = false;
	
protected:
	TObjectPtr<UStylizedSkySubsystem> GetStylizedSkySubsystem() const;
	TObjectPtr<AStylizedSkyActor> GetStylizedSkyActor() const;
	FGameplayTag GetCurrentWeatherTag() const;
	bool IsWeatherWet() const;
	bool IsOwnerSwimming() const;

	void CeilingTrace();
	void OnSingleTraceComplete(const FTraceHandle& TraceHandle, FTraceDatum& TraceDatum);
	
	void UpdateWetness(float DeltaTime);
	void UpdateMeshesParams(const float Wetness) const;
	
	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
								   FActorComponentTickFunction* ThisTickFunction) override;
};
