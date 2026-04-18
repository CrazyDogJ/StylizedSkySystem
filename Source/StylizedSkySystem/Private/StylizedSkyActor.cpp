
#include "StylizedSkyActor.h"

#include "StylizedSkySubsystem.h"
#include "StylizedSkyWeatherData.h"
#include "NiagaraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Curves/CurveLinearColor.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Net/UnrealNetwork.h"
#if WITH_EDITOR
#include "LevelEditorViewport.h"
#endif

AStylizedSkyActor::AStylizedSkyActor(const FObjectInitializer& ObjectInitializer)
{
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	
	DirectionalLight_Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
	DirectionalLight_Moon = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Moon"));

	GlobalSkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyAtmosphereComponent = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("SkyAtmosphere"));
	CloudComponent = CreateDefaultSubobject<UVolumetricCloudComponent>(TEXT("VolumetricCloud"));
	ExponentialHeightFog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("ExponentialHeightFog"));

	DirectionalLight_Sun->SetForwardShadingPriority(0);
	DirectionalLight_Moon->SetForwardShadingPriority(1);

	DirectionalLight_Sun->SetAtmosphereSunLightIndex(0);
	DirectionalLight_Moon->SetAtmosphereSunLightIndex(1);
	
	DirectionalLight_Sun->SetUseTemperature(true);
	DirectionalLight_Moon->SetUseTemperature(true);

	PostProcessComponent = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcessComp"));
	PostProcessComponent->bUnbound = true;
	
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
}

void AStylizedSkyActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Update directional light params
	UpdateSunMoon();
	UpdateSkyLight();
	ConstructCloud();
	PostProcessComponent->Settings = DefaultPostProcessSettings;
	WeatherAlpha = 0.0f;
	UpdateNiagaraAsset(true);
	UpdateAudioAsset(true);
}

void AStylizedSkyActor::BeginPlay()
{
	Super::BeginPlay();

	if (const auto Subsystem = GetWorld()->GetSubsystem<UStylizedSkySubsystem>())
	{
		Subsystem->SkyActor = this;
	}
}

void AStylizedSkyActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (const auto Subsystem = GetWorld()->GetSubsystem<UStylizedSkySubsystem>())
	{
		Subsystem->SkyActor = nullptr;
	}
}

void AStylizedSkyActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateDayTime(DeltaSeconds);
	UpdateSunMoon();
	UpdateSkyLight();
	UpdatePostProcess();
	UpdateWeather(DeltaSeconds);
}

void AStylizedSkyActor::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AStylizedSkyActor, RawDayTime)
}

UNiagaraComponent* AStylizedSkyActor::GetNiagaraComponent_Implementation() const
{
	return nullptr;
}

UAudioComponent* AStylizedSkyActor::GetAudioComponent_Implementation() const
{
	return nullptr;
}

void AStylizedSkyActor::ConstructCloud()
{
	CloudMaterialInstance = UKismetMaterialLibrary::CreateDynamicMaterialInstance(this, CloudMaterial);
	CloudComponent->SetMaterial(CloudMaterialInstance);
}

void AStylizedSkyActor::ProcessEvents(const float OldTime, const float NewTime)
{
	if (TimeTriggerTasks.IsValidIndex(TimeTriggerIndex))
	{
		const auto TriggerTime = TimeTriggerTasks[TimeTriggerIndex].TriggerTime;
		if (OldTime < TriggerTime && TriggerTime <= NewTime)
		{
			TimeTriggerTasks[TimeTriggerIndex].Trigger();
			TimeTriggerIndex++;
		}
	}
}

void AStylizedSkyActor::UpdateDayTime(float DeltaSeconds)
{
	// Loop day time float
	const float OldDayTime = RawDayTime;
	float DayTimeCopy = RawDayTime;
	const float GameDayTimeSeconds = DeltaSeconds * TimeFlowSpeed;
	DayTimeCopy += GameDayTimeSeconds;
	if (RawDayTime >= DAY_MAX_TIME)
	{
		// Reset daytime and trigger index.
		RawDayTime = DayTimeCopy - DAY_MAX_TIME;
		Day += 1;
		TimeTriggerIndex = 0;
	}
	else
	{
		RawDayTime = DayTimeCopy;
	}

	// Process time trigger event.
	ProcessEvents(OldDayTime, DayTimeCopy);
}

void AStylizedSkyActor::UpdateSkyLight() const
{
	const float NormalizedDayTime = RawDayTime / DAY_MAX_TIME;
	if (SkyLightColorCurve)
	{
		const auto Color = SkyLightColorCurve->GetLinearColorValue(NormalizedDayTime);
		GlobalSkyLight->SetLightColor(Color);
	}
	if (SkyLightDensityCurve)
	{
		const auto Density = SkyLightDensityCurve->GetFloatValue(NormalizedDayTime);
		GlobalSkyLight->SetIntensity(Density);
	}
}

void AStylizedSkyActor::UpdateSunMoonAngle() const
{
	const float NormalizedTime = FMath::Fmod(RawDayTime, DAY_MAX_TIME) / DAY_MAX_TIME;
	const float SunPitch = NormalizedTime * 360.f + 90.f; // -90° = Sun rise
	
	DirectionalLight_Sun->SetWorldRotation(FRotator(SunPitch, SunYaw, SunRoll));
	DirectionalLight_Moon->SetWorldRotation(FRotator(SunPitch + 180.f + AdditionalMoonPitch, MoonYaw, MoonRoll));

	// Priority
	if (DirectionalLight_Sun->GetComponentRotation().Pitch <= 0.f)
	{
		DirectionalLight_Sun->SetForwardShadingPriority(0);
		DirectionalLight_Moon->SetForwardShadingPriority(1);

		DirectionalLight_Sun->SetAtmosphereSunLightIndex(0);
		DirectionalLight_Moon->SetAtmosphereSunLightIndex(1);
	}
	else
	{
		DirectionalLight_Sun->SetForwardShadingPriority(1);
		DirectionalLight_Moon->SetForwardShadingPriority(0);

		DirectionalLight_Sun->SetAtmosphereSunLightIndex(1);
		DirectionalLight_Moon->SetAtmosphereSunLightIndex(0);
	}
}

void AStylizedSkyActor::UpdateSunMoonVisual() const
{
	const float NormalizedDayTime = RawDayTime / DAY_MAX_TIME;
	if (SunLightColorCurve)
	{
		DirectionalLight_Sun->SetLightColor(SunLightColorCurve->GetLinearColorValue(NormalizedDayTime));
	}
	
	if (MoonLightColorCurve)
	{
		DirectionalLight_Moon->SetLightColor(MoonLightColorCurve->GetLinearColorValue(NormalizedDayTime));
	}

	if (SunLightDensityCurve)
	{
		if (DirectionalLight_Sun->GetComponentRotation().Pitch <= 0.0f)
		{
			DirectionalLight_Sun->SetVisibility(true);
			DirectionalLight_Sun->SetIntensity(SunLightDensityCurve->GetFloatValue(NormalizedDayTime));
			DirectionalLight_Sun->SetCastShadows(true);
		}
		else
		{
			// Avoid underground directional light.
			DirectionalLight_Sun->SetVisibility(false);
			DirectionalLight_Sun->SetCastShadows(false);
		}
	}

	if (MoonLightDensityCurve)
	{
		if (DirectionalLight_Moon->GetComponentRotation().Pitch < 0.0f)
		{
			DirectionalLight_Moon->SetVisibility(true);
			DirectionalLight_Moon->SetIntensity(MoonLightDensityCurve->GetFloatValue(NormalizedDayTime));
			DirectionalLight_Moon->SetCastShadows(true);
		}
		else
		{
			// Avoid underground directional light.
			DirectionalLight_Moon->SetVisibility(false);
			DirectionalLight_Moon->SetCastShadows(false);
		}
	}

	if (SunLightTemperature)
	{
		DirectionalLight_Sun->SetTemperature(SunLightTemperature->GetFloatValue(NormalizedDayTime));
	}
	
	if (MoonLightTemperature)
	{
		DirectionalLight_Moon->SetTemperature(MoonLightTemperature->GetFloatValue(NormalizedDayTime));
	}
}

void AStylizedSkyActor::UpdateSunMoon() const
{
	UpdateSunMoonAngle();
	UpdateSunMoonVisual();
}

void AStylizedSkyActor::UpdatePostProcess() const
{
	PostProcessComponent->Settings = DefaultPostProcessSettings;
}

void AStylizedSkyActor::UpdateNiagaraAsset(const bool ClearAsset) const
{
	if (const auto NiagaraComp = GetNiagaraComponent())
	{
		if (WeatherPreset && WeatherPreset->WeatherNiagaraSystem)
		{
			NiagaraComp->SetAsset(WeatherPreset->WeatherNiagaraSystem);
			NiagaraComp->Activate(true);
		}
		else
		{
			NiagaraComp->Deactivate();
			if (ClearAsset)
			{
				NiagaraComp->SetAsset(nullptr);
			}
		}
	}
}

void AStylizedSkyActor::UpdateAudioAsset(const bool ClearAsset) const
{
	if (const auto AudioComp = GetAudioComponent())
	{
		if (WeatherPreset)
		{
			AudioComp->SetSound(WeatherPreset->WeatherSound);
			AudioComp->FadeIn(WeatherPreset->FadeTime);
		}
		else if (PreviousWeatherPreset)
		{
			AudioComp->FadeOut(PreviousWeatherPreset->FadeTime, 0.0f);
		}
		else
		{
			AudioComp->FadeOut(5.0f, 0.0f);
			if (ClearAsset)
			{
				AudioComp->SetSound(nullptr);
			}
		}
	}
}

void AStylizedSkyActor::SetWeatherPreset(UStylizedSkyWeatherData* InWeatherData)
{
	if (WeatherPreset != InWeatherData)
	{
		PreviousWeatherPreset = WeatherPreset;
		WeatherPreset = InWeatherData;
		WeatherAlpha = 0.0f;
		UpdateNiagaraAsset();
		UpdateAudioAsset();
	}
}

void AStylizedSkyActor::UpdateWeather(float DeltaTime)
{
	const auto Delta = DeltaTime * (WeatherPreset ? WeatherPreset->GetFadeAlphaSpeed() : 1.0f / 5.0f);
	WeatherAlpha = FMath::Clamp(WeatherAlpha + Delta, 0.0f, 1.0f);

	// TODO : Interp Niagara Params for interp visual.
	
	// Interp cloud
	CloudMaterialInstance->K2_InterpolateMaterialInstanceParams(GetPreviousCloudMaterialInstance(), GetCurrentCloudMaterialInstance(), WeatherAlpha);

	// Interp fog
	FExponentialHeightFogData PrevFogData;
	FLinearColor PrevFogColor;
	GetPreviousFogSettings(PrevFogData, PrevFogColor);
	FExponentialHeightFogData FogData;
	FLinearColor FogColor;
	GetCurrentFogSettings(FogData, FogColor);
	ExponentialHeightFog->SetSecondFogDensity(FMath::Lerp(PrevFogData.FogDensity, FogData.FogDensity, WeatherAlpha));
	ExponentialHeightFog->SetSecondFogHeightFalloff(FMath::Lerp(PrevFogData.FogHeightFalloff, FogData.FogHeightFalloff, WeatherAlpha));
	ExponentialHeightFog->SetSecondFogHeightOffset(FMath::Lerp(PrevFogData.FogHeightOffset, FogData.FogHeightOffset, WeatherAlpha));
	ExponentialHeightFog->SetFogInscatteringColor(FMath::Lerp(PrevFogColor, FogColor, WeatherAlpha));

	// Interp light
	const float NormalizedDayTime = RawDayTime / DAY_MAX_TIME;
	float PrevDensity;
	FLinearColor PrevColor;
	GetPreviousLightSettings(PrevDensity, PrevColor);
	float Density;
	FLinearColor Color;
	GetCurrentLightSettings(Density, Color);
	const float SunIntensity = SunLightDensityCurve->GetFloatValue(NormalizedDayTime);
	const float MoonIntensity = MoonLightDensityCurve->GetFloatValue(NormalizedDayTime);
	const float SkylightIntensity = SkyLightDensityCurve->GetFloatValue(NormalizedDayTime);
	DirectionalLight_Sun->SetIntensity(FMath::Lerp(SunIntensity * PrevDensity, SunIntensity * Density, WeatherAlpha));
	DirectionalLight_Moon->SetIntensity(FMath::Lerp(MoonIntensity * PrevDensity, MoonIntensity * Density, WeatherAlpha));
	GlobalSkyLight->SetIntensity(FMath::Lerp(SkylightIntensity * PrevDensity, SkylightIntensity * Density, WeatherAlpha));
}

UMaterialInstance* AStylizedSkyActor::GetPreviousCloudMaterialInstance() const
{
	return PreviousWeatherPreset ? PreviousWeatherPreset->WeatherCloudMaterialInstance : CloudMaterial;
}

UMaterialInstance* AStylizedSkyActor::GetCurrentCloudMaterialInstance() const
{
	return WeatherPreset ? WeatherPreset->WeatherCloudMaterialInstance : CloudMaterial;
}

void AStylizedSkyActor::GetPreviousFogSettings(FExponentialHeightFogData& OutData,
	FLinearColor& InscatteringLuminance) const
{
	const auto Default = Cast<AStylizedSkyActor>(GetClass()->GetDefaultObject());
	const auto DefaultSecondFogData = Default->ExponentialHeightFog->SecondFogData;
	const auto DefaultFogInscatteringLuminance = Default->ExponentialHeightFog->FogInscatteringLuminance;
	OutData = PreviousWeatherPreset ? PreviousWeatherPreset->SecondFogData : DefaultSecondFogData;
	InscatteringLuminance = PreviousWeatherPreset ? PreviousWeatherPreset->FogInscatteringLuminance : DefaultFogInscatteringLuminance;
}

void AStylizedSkyActor::GetCurrentFogSettings(FExponentialHeightFogData& OutData,
	FLinearColor& InscatteringLuminance) const
{
	const auto Default = Cast<AStylizedSkyActor>(GetClass()->GetDefaultObject());
	const auto DefaultSecondFogData = Default->ExponentialHeightFog->SecondFogData;
	const auto DefaultFogInscatteringLuminance = Default->ExponentialHeightFog->FogInscatteringLuminance;
	OutData = WeatherPreset ? WeatherPreset->SecondFogData : DefaultSecondFogData;
	InscatteringLuminance = WeatherPreset ? WeatherPreset->FogInscatteringLuminance : DefaultFogInscatteringLuminance;
}

void AStylizedSkyActor::GetPreviousLightSettings(float& DensityMultiplier, FLinearColor& ColorMultiplier) const
{
	DensityMultiplier = PreviousWeatherPreset ? PreviousWeatherPreset->LightMultiplier : 1.0f;
	ColorMultiplier = PreviousWeatherPreset ? PreviousWeatherPreset->LightColorMultiplier : FLinearColor::White;
}

void AStylizedSkyActor::GetCurrentLightSettings(float& DensityMultiplier, FLinearColor& ColorMultiplier) const
{
	DensityMultiplier = WeatherPreset ? WeatherPreset->LightMultiplier : 1.0f;
	ColorMultiplier = WeatherPreset ? WeatherPreset->LightColorMultiplier : FLinearColor::White;
}

#if WITH_EDITOR
bool AStylizedSkyActor::GetLevelViewportCameraInfo(FVector& CameraLocation, FRotator& CameraRotation)
{
	bool RetVal = false;
	CameraLocation = FVector::ZeroVector;
	CameraRotation = FRotator::ZeroRotator;

	for (FLevelEditorViewportClient* LevelVC : GEditor->GetLevelViewportClients())
	{
		if (LevelVC && LevelVC->IsPerspective())
		{
			CameraLocation = LevelVC->GetViewLocation();
			CameraRotation = LevelVC->GetViewRotation();
			RetVal = true;

			break;
		}
	}

	return RetVal;
}
#endif

bool AStylizedSkyActor::GetEditorViewportCameraInfo(FVector& Location, FRotator& Rotation) const
{
	if (GetWorld()->IsGameWorld())
	{
		if (const auto LocalController = GetWorld()->GetFirstLocalPlayerFromController())
		{
			if (LocalController->PlayerController && LocalController->PlayerController->PlayerCameraManager)
			{
				Location = LocalController->PlayerController->PlayerCameraManager->GetCameraLocation();
				Rotation = LocalController->PlayerController->PlayerCameraManager->GetCameraRotation();
				return true;
			}
		}
	}

#if WITH_EDITOR
	const auto CameraValid = GetLevelViewportCameraInfo(Location, Rotation);
	return CameraValid;
#else
	return false;
#endif
}

FTimespan AStylizedSkyActor::GetDayTime() const
{
	return FTimespan::FromSeconds(RawDayTime);
}

FDateTime AStylizedSkyActor::GetDateTime() const
{
	auto NewDateTime = FDateTime();
	const auto NewDayTimeSpan = FTimespan::FromDays(Day);
	const auto NewDayTime = GetDayTime();
	NewDateTime += NewDayTimeSpan + NewDayTime;
	return NewDateTime;
}

FDateTime AStylizedSkyActor::SecondsToDateTime(double InSeconds)
{
	// 先构造一个时间跨度
	FTimespan TimeSpan = FTimespan::FromSeconds(InSeconds);

	// 以 0001-01-01 00:00:00 为基准，加上 TimeSpan
	return FDateTime(1, 1, 1, 0, 0, 0) + TimeSpan;
}

float AStylizedSkyActor::DateTimeToSeconds(FDateTime DateTime)
{
	const auto TimeSpan = FTimespan(DateTime.GetHour(), DateTime.GetMinute(), DateTime.GetSecond());
	return TimeSpan.GetTotalSeconds();
}

void AStylizedSkyActor::InsertTask(const float InRawDayTime, const FTimeTriggerTask& TriggerTask)
{
	if (TimeTriggerTasks.IsEmpty())
	{
		TimeTriggerTasks.Add(TriggerTask);
	}
	
	for (int i = 0; i < TimeTriggerTasks.Num(); ++i)
	{
		if (TimeTriggerTasks[i].TriggerTime < InRawDayTime)
		{
			if (TimeTriggerTasks.IsValidIndex(i + 1))
			{
				if (InRawDayTime < TimeTriggerTasks[i + 1].TriggerTime)
				{
					// If insert time is behind, we add 1;
					if (InRawDayTime < RawDayTime)
					{
						TimeTriggerIndex++;
					}
					
					TimeTriggerTasks.Insert(TriggerTask, i + 1);
					return;
				}
			}
			else
			{
				TimeTriggerTasks.Add(TriggerTask);
				return;
			}
		}
	}
}

void AStylizedSkyActor::RegisterTimeTriggerEvent(const float InRawDayTime, FTimeTriggerEvent TriggerEvent)
{
	const auto Found = TimeTriggerTasks.FindByPredicate([InRawDayTime](const FTimeTriggerTask& TriggerTask)
	{
		return TriggerTask.TriggerTime == InRawDayTime;
	});

	if (Found)
	{
		Found->TriggerEvents.Add(TriggerEvent);
	}
	else
	{
		auto NewTask = FTimeTriggerTask();
		NewTask.TriggerTime = InRawDayTime;
		NewTask.TriggerEvents.Add(TriggerEvent);
		InsertTask(InRawDayTime, NewTask);
	}
}

void AStylizedSkyActor::UnregisterTimeTriggerEvent(FTimeTriggerEvent TriggerEvent)
{
	TArray<int> EmptyIndices;
	for (int i = 0; i < TimeTriggerTasks.Num(); ++i)
	{
		auto& Itr = TimeTriggerTasks[i];
		Itr.TriggerEvents.Remove(TriggerEvent);
		if (Itr.TriggerEvents.IsEmpty())
		{
			EmptyIndices.Add(i);
		}
	}

	for (const auto Itr : EmptyIndices)
	{
		if (TimeTriggerTasks[Itr].TriggerTime < RawDayTime)
		{
			TimeTriggerIndex--;
		}
		
		TimeTriggerTasks.RemoveAt(Itr, EAllowShrinking::No);
	}

	TimeTriggerTasks.Shrink();
}

#if WITH_EDITOR
bool AStylizedSkyActor::ShouldTickIfViewportsOnly() const
{
	return true;
}

void AStylizedSkyActor::TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	if (!GetWorld()->IsGameWorld())
	{
		FEditorScriptExecutionGuard ScriptGuard;
		EditorTick(DeltaTime);
		UpdateSunMoon();
		UpdateSkyLight();
		UpdatePostProcess();
		UpdateWeather(DeltaTime);
	}
	else
	{
		Super::TickActor(DeltaTime, TickType, ThisTickFunction);
	}
}
#endif
