// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/LightDetector.h"

// Sets default values
ALightDetector::ALightDetector()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create root component
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// Create Top Scene Capture
	SceneCaptureTop = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCaptureTop"));
	SceneCaptureTop->SetupAttachment(RootComponent);
	SceneCaptureTop->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));
	SceneCaptureTop->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f)); // Look down
	SceneCaptureTop->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	SceneCaptureTop->bCaptureEveryFrame = false;
	SceneCaptureTop->bCaptureOnMovement = false;

	// Create Bottom Scene Capture
	SceneCaptureBottom = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCaptureBottom"));
	SceneCaptureBottom->SetupAttachment(RootComponent);
	SceneCaptureBottom->SetRelativeLocation(FVector(0.0f, 0.0f, -50.0f));
	SceneCaptureBottom->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f)); // Look up
	SceneCaptureBottom->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	SceneCaptureBottom->bCaptureEveryFrame = false;
	SceneCaptureBottom->bCaptureOnMovement = false;

	// Initialize
	BrightnessOutput = 0.0f;
	NextUpdateTime = 0.0f;
}

// Called when the game starts or when spawned
void ALightDetector::BeginPlay()
{
	Super::BeginPlay();

	// Auto-create RenderTargets if not assigned
	if (!DetectorTextureTop)
	{
		DetectorTextureTop = NewObject<UTextureRenderTarget2D>(this);
		DetectorTextureTop->InitAutoFormat(128, 128);
		DetectorTextureTop->UpdateResourceImmediate(true);
		UE_LOG(LogTemp, Log, TEXT("LightDetector: Auto-created DetectorTextureTop"));
	}

	if (!DetectorTextureBottom)
	{
		DetectorTextureBottom = NewObject<UTextureRenderTarget2D>(this);
		DetectorTextureBottom->InitAutoFormat(128, 128);
		DetectorTextureBottom->UpdateResourceImmediate(true);
		UE_LOG(LogTemp, Log, TEXT("LightDetector: Auto-created DetectorTextureBottom"));
	}
	
	// Assign render targets to scene captures
	if (DetectorTextureTop && SceneCaptureTop)
	{
		SceneCaptureTop->TextureTarget = DetectorTextureTop;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("LightDetector: DetectorTextureTop is not assigned!"));
	}

	if (DetectorTextureBottom && SceneCaptureBottom)
	{
		SceneCaptureBottom->TextureTarget = DetectorTextureBottom;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("LightDetector: DetectorTextureBottom is not assigned!"));
	}
}

// Called every frame
void ALightDetector::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bAutoUpdate)
	{
		return;
	}

	float CurrentTime = GetWorld()->GetTimeSeconds();

	if (CurrentTime >= NextUpdateTime)
	{
		NextUpdateTime = CurrentTime + UpdateInterval;
		CalculateBrightness();

		// Debug
		if (bShowDebugInfo && GEngine)
		{
			FColor DebugColor = FColor::MakeRedToGreenColorFromScalar(1.0f - BrightnessOutput);
			GEngine->AddOnScreenDebugMessage(200, 0.0f, DebugColor,
				FString::Printf(TEXT("Light Brightness: %.3f"), BrightnessOutput));
		}
	}
}

void ALightDetector::ProcessRenderTexture(UTextureRenderTarget2D* Texture)
{
	if (!Texture)
	{
		return;
	}

	// Get render target resource
	FRenderTarget* RenderTarget = Texture->GameThread_GetRenderTargetResource();
	if (!RenderTarget)
	{
		UE_LOG(LogTemp, Warning, TEXT("LightDetector: Failed to get RenderTarget resource"));
		return;
	}

	// Read pixels from render target
	RenderTarget->ReadPixels(PixelStorage);

	// Find the brightest pixel
	for (int32 PixelIndex = 0; PixelIndex < PixelStorage.Num(); PixelIndex++)
	{
		const FColor& Pixel = PixelStorage[PixelIndex];

		float PixelChannelR = static_cast<float>(Pixel.R);
		float PixelChannelG = static_cast<float>(Pixel.G);
		float PixelChannelB = static_cast<float>(Pixel.B);

		// Calculate brightness using standard luminance formula
		// Human eye perception: Green (58.7%) > Red (29.9%) > Blue (11.4%)
		float CurrentPixelBrightness = (0.299f * PixelChannelR) + (0.587f * PixelChannelG) + (0.114f * PixelChannelB);

		// Normalize to 0-1 range (0-255 → 0-1)
		CurrentPixelBrightness /= 255.0f;

		// Update if this pixel is brighter
		if (CurrentPixelBrightness > BrightnessOutput)
		{
			BrightnessOutput = CurrentPixelBrightness;
		}
	}
}

float ALightDetector::CalculateBrightness()
{
	// Validate render textures
	if (!DetectorTextureTop || !DetectorTextureBottom)
	{
		UE_LOG(LogTemp, Warning, TEXT("LightDetector: Render textures not assigned!"));
		return 0.0f;
	}

	// Capture scenes
	if (SceneCaptureTop)
	{
		SceneCaptureTop->CaptureScene();
	}

	if (SceneCaptureBottom)
	{
		SceneCaptureBottom->CaptureScene();
	}

	// Reset brightness
	BrightnessOutput = 0.0f;

	// Process both textures
	ProcessRenderTexture(DetectorTextureTop);
	ProcessRenderTexture(DetectorTextureBottom);

	// Clamp to 0-1 range
	BrightnessOutput = FMath::Clamp(BrightnessOutput, 0.0f, 1.0f);

	return BrightnessOutput;
}