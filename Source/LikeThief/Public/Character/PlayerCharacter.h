// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Components/TimelineComponent.h"
#include "PlayerCharacter.generated.h"

class UCmaeraComponent;
class UCurveFloat;

UCLASS()
class LIKETHIEF_API APlayerCharacter : public ACharacter
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* Camera;

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBloom;

public:
	// Sets default values for this character's properties
	APlayerCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	class UInputMappingContext* InputMapping;

	// Move Input Actions
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	class UInputAction* MoveAction;

	// Look Input Actions
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	class UInputAction* LookAction;

	// Jump Input Actions
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	class UInputAction* JumpAction;

	// Jump Input Actions
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	class UInputAction* CrouchAction;

	// Lean Input Actions
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	class UInputAction* LeanLeftAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	class UInputAction* LeanRightAction;

	//--- walkspeed
	class UCharacterMovementComponent* CharacterMovement = GetCharacterMovement();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Move")
	float DefaultMovementSpeed = 500.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Move")
	float CrouchMovementSpeed = 300.0f;

	// ---Crouch---
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Crouch")
	float DefaultCapsuleHalfHeight = 88.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Crouch")
	float CrouchingHalfHeight = 44.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Crouch")
	UCurveFloat* CrouchingCurve = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Crouch")
	float TraceRadius = 40.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Crouch")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Crouch")
	float CameraHeightMultiplier = 0.67f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Crouch")
	FVector DefaultCameraBloomLocation = FVector(0.0f, 0.0f, 60.0f);

	// ---Lean---
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Lean")
	UCurveFloat* LeanCurve = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Lean")
	FTransform DefaultCameraTransform = FTransform(FRotator(0.0f, 0.0f, 0.0f), FVector(0.0f, 0.0f, 0.0f), FVector(1.0f, 1.0f, 1.0f));

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Lean")
	FTransform LeanLeftTransform = FTransform(FRotator(0.0f, 0.0f, -20.0f), FVector(0.0f, -15.0f, 0.0f), FVector(1.0f, 1.0f, 1.0f));

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Lean")
	FTransform LeanRightTransform = FTransform(FRotator(0.0f, 0.0f, 20.0f), FVector(0.0f, 15.0f, 0.0f), FVector(1.0f, 1.0f, 1.0f));

	// ---Mantle---
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Mantle")
	UCurveFloat* MantleCurve = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Mantle")
	float MantleCheckDelay = 0.001f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Mantle")
	float MantleForwardDistance = 17.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Mantle")
	float MantleSphereRadius = 34.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Mantle")
	float MantleOverheadCheckRadius = 17.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Mantle")
	float MantleOverheadCheckHeight = 100.0f;
protected:
	// Handles Movement Input
	void Move(const FInputActionValue& InputValue);

	// Handles Look Input
	void Look(const FInputActionValue& InputValue);

	void Jump();
	void StopJump();

	// --- Crouch---
public:
	FTimeline CrouchingTimeline;

	bool bIsCrouching = false;

	UFUNCTION()
	void CrouchUpdate(float Alpha);

	UFUNCTION()
	void CrouchFinished();

	void StartCrouch();
	void StopCrouch();
	void ToggleCrouch();

	bool CanStandUp();

	// --- Lean---
	FTimeline LeanLeftTimeline;
	FTimeline LeanRightTimeline;


	UPROPERTY(BlueprintReadWrite, Category = "Lean")
	float LeanValue = 0.0f;

	UFUNCTION()
	void LeanLeftUpdate(float Alpha);

	UFUNCTION()
	void LeanLeftFinished();

	UFUNCTION()
	void LeanRightUpdate(float Alpha);

	UFUNCTION()
	void LeanRightFinished();

	void StartLeanLeft();
	void StopLeanLeft();

	void StartLeanRight();
	void StopLeanRight();

	// --- Mantle ---
	FTimeline MantleTimeline;

	bool bHold = false;
	bool bHitDetected = false;
	bool bIsMantling = false;
	FVector MantleTargetLocation = FVector::ZeroVector;
	FTimerHandle MantleCheckTimerHandle;

	UFUNCTION()
	void MantleUpdate(float Alpha);

	UFUNCTION()
	void MantleFinished();

	void CheckMantleCondition();
	void MantleCheck();
	void MantleUp();
	bool CheckMantleOverhead();
	void CancelMantle();
};
