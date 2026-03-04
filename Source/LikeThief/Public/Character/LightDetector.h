// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/SceneCaptureComponent2D.h"
#include "LightDetector.generated.h"

UCLASS()
class LIKETHIEF_API ALightDetector : public AActor
{
	GENERATED_BODY()

public:	
	// Sets default values for this actor's properties
	ALightDetector();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Get normalized brightness (0.0 ~ 1.0)
	UFUNCTION(BlueprintPure, Category = "LightDetection")
	float GetBrightness() const { return BrightnessOutput; }

	// Calculate brightness manually
	UFUNCTION(BlueprintCallable, Category = "LightDetection")
	float CalculateBrightness();

	// Scene Capture Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneCaptureComponent2D* SceneCaptureTop;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneCaptureComponent2D* SceneCaptureBottom;

	// Render Textures
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LightDetection")
	UTextureRenderTarget2D* DetectorTextureTop;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LightDetection")
	UTextureRenderTarget2D* DetectorTextureBottom;

	// Settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LightDetection")
	bool bAutoUpdate = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LightDetection")
	float UpdateInterval = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LightDetection")
	bool bShowDebugInfo = false;


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	void ProcessRenderTexture(UTextureRenderTarget2D* Texture);

	TArray<FColor> PixelStorage;
	float BrightnessOutput = 0.0f;
	float NextUpdateTime = 0.0f;


};
