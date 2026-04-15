
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/Interface_PostProcessVolume.h"
#include "Rendering/ExponentialHeightFogData.h"
#include "StylizedSkyActor.generated.h"

class UNiagaraComponent;
class UStylizedSkyWeatherData;
class UPostProcessComponent;
class UExponentialHeightFogComponent;
class UVolumetricCloudComponent;
class USkyAtmosphereComponent;
class UCurveLinearColor;
class UCurveFloat;

// Day time seconds;
constexpr float DAY_MAX_TIME = 86400.0f;

DECLARE_DYNAMIC_DELEGATE_OneParam(FTimeTriggerEvent, float, TriggerTime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTimeTriggerEventMulticast, float, TriggerTime);

USTRUCT(BlueprintType)
struct FTimeTriggerTask
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	float TriggerTime = -1.0f;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	TArray<FTimeTriggerEvent> TriggerEvents;

	void Trigger() const
	{
		for (const auto Itr : TriggerEvents)
		{
			// ReSharper disable once CppExpressionWithoutSideEffects
			Itr.ExecuteIfBound(TriggerTime);
		}
	}
};

UCLASS(Blueprintable)
class STYLIZEDSKYSYSTEM_API AStylizedSkyActor : public AActor
{
	GENERATED_BODY()

public:
	
	// Construct function.
	explicit AStylizedSkyActor(const FObjectInitializer& ObjectInitializer);

	// Default Components ------------------------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Basic")
	USceneComponent* Root;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lights")
	UDirectionalLightComponent* DirectionalLight_Sun;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lights")
	UDirectionalLightComponent* DirectionalLight_Moon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lights")
	USkyLightComponent* GlobalSkyLight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sky")
	USkyAtmosphereComponent* SkyAtmosphereComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sky")
	UVolumetricCloudComponent* CloudComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sky")
	UExponentialHeightFogComponent* ExponentialHeightFog;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Post Process")
	UPostProcessComponent* PostProcessComponent;

	// Properties ------------------------------------------------------------------------------------

	/** Float day time, easy to use in coding. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, meta=( PropertyVisible, UIMin = 0, UIMax = 86400, ClampMin = 0, ClampMax = 86400 ), Category = "Day Time")
	float RawDayTime = 43200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, meta=( PropertyVisible, UIMin = 0, ClampMin = 0 ), Category = "Day Time")
	int Day = 1; 

	/** Day time flowing speed. Default will be 60, that means 1s real time equals to 1min game time. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=( PropertyVisible ), Category = "Day Time")
	float TimeFlowSpeed = 60.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=( PropertyVisible ), Category = "Sun")
	UCurveLinearColor* SunLightColorCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,	meta=( PropertyVisible ), Category = "Sun")
	UCurveFloat* SunLightDensityCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=( PropertyVisible ), Category = "Sun")
	UCurveFloat* SunLightTemperature;

	/** Season? Earth location? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=( PropertyVisible ), Category = "Sun")
	float SunYaw = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=( PropertyVisible ), Category = "Sun")
	float SunRoll = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=( PropertyVisible ), Category = "Moon")
	UCurveLinearColor* MoonLightColorCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,	meta=( PropertyVisible ), Category = "Moon")
	UCurveFloat* MoonLightDensityCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=( PropertyVisible ), Category = "Moon")
	UCurveFloat* MoonLightTemperature;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=( PropertyVisible ), Category = "Moon")
	float AdditionalMoonPitch = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=( PropertyVisible ), Category = "Moon")
	float MoonYaw = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=( PropertyVisible ), Category = "Moon")
	float MoonRoll = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=( PropertyVisible ), Category = "Sky Light")
	UCurveLinearColor* SkyLightColorCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,	meta=( PropertyVisible ), Category = "Sky Light")
	UCurveFloat* SkyLightDensityCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=( PropertyVisible ), Category = "Cloud")
	UMaterialInstance* CloudMaterial;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=( PropertyVisible ), Category = "Cloud")
	UMaterialInstanceDynamic* CloudMaterialInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=( PropertyVisible ), Category = "Post Process")
	FPostProcessSettings DefaultPostProcessSettings;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Time Trigger")
	TArray<FTimeTriggerTask> TimeTriggerTasks;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Time Trigger")
	int TimeTriggerIndex = 0;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, meta=( PropertyVisible ), Replicated, Category = "Weather")
	UStylizedSkyWeatherData* PreviousWeatherPreset = nullptr;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=( PropertyVisible ), Replicated, BlueprintSetter = SetWeatherPreset, Category = "Weather")
	UStylizedSkyWeatherData* WeatherPreset = nullptr;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, meta=( PropertyVisible ), Replicated, Category = "Weather")
	float WeatherAlpha = 0.0f;
	
	// Helper functions ------------------------------------------------------------------------------------
	UFUNCTION(BlueprintPure)
	bool GetEditorViewportCameraInfo(FVector& Location, FRotator& Rotation) const;

	UFUNCTION(BlueprintPure)
	FTimespan GetDayTime() const;

	UFUNCTION(BlueprintPure)
	FDateTime GetDateTime() const;

	UFUNCTION(BlueprintPure)
	FDateTime SecondsToDateTime(double InSeconds);

	UFUNCTION(BlueprintPure)
	float DateTimeToSeconds(FDateTime DateTime);

	// Time Manager Task -----------------------------------------------------------------------------------
	void InsertTask(const float InRawDayTime, const FTimeTriggerTask& TriggerTask);
	
	UFUNCTION(BlueprintCallable)
	void RegisterTimeTriggerEvent(const float InRawDayTime, FTimeTriggerEvent TriggerEvent);

	UFUNCTION(BlueprintCallable)
	void UnregisterTimeTriggerEvent(FTimeTriggerEvent TriggerEvent);

	UFUNCTION(BlueprintNativeEvent)
	UNiagaraComponent* GetNiagaraComponent() const;

	UFUNCTION(BlueprintNativeEvent)
	UAudioComponent* GetAudioComponent() const;
	
#if WITH_EDITOR
	virtual bool ShouldTickIfViewportsOnly() const override;
	virtual void TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;

	UFUNCTION(BlueprintImplementableEvent)
	void EditorTick(float DeltaTime);
#endif

private:
	// Override Functions ------------------------------------------------------------------------------------
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	// Functions ------------------------------------------------------------------------------------

	void ConstructCloud();
	
	/** Process time events. */
	void ProcessEvents(const float OldTime, const float NewTime);
	
	/** Time flowing. */
	void UpdateDayTime(float DeltaSeconds);

	/** Update skylight params*/
	void UpdateSkyLight() const;
	
	/** Update sun and moon angles. */
	void UpdateSunMoonAngle() const;

	/** Update sun and moon visual. */
	void UpdateSunMoonVisual() const;
	
	/** Update sun and moon. */
	void UpdateSunMoon() const;

	void UpdatePostProcess() const;
	
	void UpdateNiagaraAsset(const bool ClearAsset = false) const;

	void UpdateAudioAsset(const bool ClearAsset = false) const;
	
	UFUNCTION(BlueprintCallable)
	void SetWeatherPreset(UStylizedSkyWeatherData* InWeatherData);

	void UpdateWeather(float DeltaTime);

	UMaterialInstance* GetPreviousCloudMaterialInstance() const;
	UMaterialInstance* GetCurrentCloudMaterialInstance() const;

	void GetPreviousFogSettings(FExponentialHeightFogData& OutData, FLinearColor& InscatteringLuminance) const;
	void GetCurrentFogSettings(FExponentialHeightFogData& OutData, FLinearColor& InscatteringLuminance) const;

	void GetPreviousLightSettings(float& DensityMultiplier, FLinearColor& ColorMultiplier) const;
	void GetCurrentLightSettings(float& DensityMultiplier, FLinearColor& ColorMultiplier) const;
	
#if WITH_EDITOR
	static bool GetLevelViewportCameraInfo(FVector& CameraLocation, FRotator& CameraRotation);
#endif
};
