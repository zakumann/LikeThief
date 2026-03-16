// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class ALightDetector;

UENUM(BlueprintType)
enum class EStealthState : uint8
{
	FullyStealth UMETA(DisplayName = "Fully Stealth"),
	PartiallyStealth UMETA(DisplayName = "Partially Stealth"),
	Exposed UMETA(DisplayName = "Exposed")
};

UCLASS()
class LIKETHIEF_API APlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	APlayerCharacter();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void Landed(const FHitResult& Hit) override;

	// === Camera ===
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* Camera;

	// === Input ===
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* CrouchAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* LeanLeftAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* LeanRightAction;

	// === Movement Functions ===
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartCrouch();
	void StopCrouch();
	void StartLeanLeft();
	void StopLeanLeft();
	void StartLeanRight();
	void StopLeanRight();
	void StartJump();
	void StopJump();

	// === Crouch System ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Crouch")
	float CrouchSpeed = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Crouch")
	float StandingCapsuleHalfHeight = 88.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Crouch")
	float CrouchingCapsuleHalfHeight = 44.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Crouch")
	bool bIsCrouching = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Crouch")
	bool bIsCrouchingInterpolating = false;

	float CrouchInterpolationTime = 0.0f;

	void UpdateCrouch(float DeltaTime);

	// === Lean System ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Lean")
	float LeanSpeed = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Lean")
	float LeanAngle = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Lean")
	float LeanDistance = 30.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Lean")
	bool bIsLeaningLeft = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Lean")
	bool bIsLeaningRight = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Lean")
	bool bIsLeaningInterpolating = false;

	float LeanInterpolationTime = 0.0f;
	FRotator TargetLeanRotation;
	FVector TargetLeanLocation;

	void UpdateLean(float DeltaTime);

	// === Mantle System ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Mantle")
	float MantleCheckDistance = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Mantle")
	float MantleHeight = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Mantle")
	float JumpHoldTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Mantle")
	float JumpHoldThreshold = 0.3f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Mantle")
	bool bIsMantling = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Mantle")
	bool bIsHoldingJump = false;

	void CheckMantle();
	void PerformMantle(FVector MantleLocation);

	// === Stealth System ===
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stealth")
	EStealthState CurrentStealthState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	float FullyStealthThreshold = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	float PartiallyStealthThreshold = 0.6f;

	UFUNCTION(BlueprintCallable, Category = "Stealth")
	float GetStealthDetectionMultiplier() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Stealth")
	float GetLightValue() const;

	void UpdateStealthState();

	// === LightDetector ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth")
	TSubclassOf<AActor> LightDetectorClass;

	UPROPERTY()
	AActor* LightDetector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stealth")
	float CurrentLightValue = 1.0f;

	// === Noise System ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise")
	float FootstepNoiseInterval = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise")
	float WalkNoiseLoudness = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise")
	float RunNoiseLoudness = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise")
	float LandingNoiseLoudness = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise")
	USoundBase* FootstepSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise")
	USoundBase* LandingSound;

	float TimeSinceLastFootstep = 0.0f;
	bool bWasInAir = false;

	void MakeMovementNoise();
	bool ShouldMakeNoise() const;
	float GetCurrentNoiseLoudness() const;
	void PlayFootstepSound();
	void MakeLandingNoise();
	void PlayLandingSound();
};