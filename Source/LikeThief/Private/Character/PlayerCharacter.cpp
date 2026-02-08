// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/PlayerCharacter.h"
#include "InputMappingContext.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Curves/CurveFloat.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetSystemLibrary.h"

// Sets default values
APlayerCharacter::APlayerCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	CameraBloom = CreateDefaultSubobject<USpringArmComponent>("CameraBloom");
	CameraBloom->SetupAttachment(RootComponent);
	CameraBloom->SetRelativeLocation(FVector(0.0f, 0.0f, 60.0f));
	CameraBloom->TargetArmLength = 0.0f;
	CameraBloom->bUsePawnControlRotation = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraBloom);
	Camera->bUsePawnControlRotation = false;
}

// Called when the game starts or when spawned
void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(InputMapping, 0);
		}
	}

	DefaultCameraBloomLocation = CameraBloom->GetRelativeLocation();

	// Crouch Timeline
	if (CrouchingCurve)
	{
		FOnTimelineFloat CrouchProgressUpdate;
		CrouchProgressUpdate.BindUFunction(this, FName("CrouchUpdate"));

		FOnTimelineEvent CrouchFinishedEvent;
		CrouchFinishedEvent.BindUFunction(this, FName("CrouchFinished"));

		CrouchingTimeline.AddInterpFloat(CrouchingCurve, CrouchProgressUpdate);
		CrouchingTimeline.SetTimelineFinishedFunc(CrouchFinishedEvent);
	}

	CharacterMovement->MaxWalkSpeed = DefaultMovementSpeed;

	// Setup Lean Left Timeline
	if (LeanCurve) {
		FOnTimelineFloat LeanLeftProgressUpdate;
		LeanLeftProgressUpdate.BindUFunction(this, FName("LeanLeftUpdate"));

		FOnTimelineEvent LeanLeftFinishedEvent;
		LeanLeftFinishedEvent.BindUFunction(this, FName("LeanLeftFinished"));

		LeanLeftTimeline.AddInterpFloat(LeanCurve, LeanLeftProgressUpdate);
		LeanLeftTimeline.SetTimelineFinishedFunc(LeanLeftFinishedEvent);


	}		
	// Setup Lean Right Timeline
	if (LeanCurve)
	{
		FOnTimelineFloat LeanRightProgressUpdate;
		LeanRightProgressUpdate.BindUFunction(this, FName("LeanRightUpdate"));

		FOnTimelineEvent LeanRightFinishedEvent;
		LeanRightFinishedEvent.BindUFunction(this, FName("LeanRightFinished"));

		LeanRightTimeline.AddInterpFloat(LeanCurve, LeanRightProgressUpdate);
		LeanRightTimeline.SetTimelineFinishedFunc(LeanRightFinishedEvent);
	}
}

// Called every frame
void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CrouchingTimeline.TickTimeline(DeltaTime);
	LeanLeftTimeline.TickTimeline(DeltaTime);
	LeanRightTimeline.TickTimeline(DeltaTime);
}

// Called to bind functionality to input
void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInput = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Move);
		EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Look);
		EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &APlayerCharacter::Jump);

		// Crouch
		EnhancedInput->BindAction(CrouchAction, ETriggerEvent::Started, this, &APlayerCharacter::ToggleCrouch);

		// Lean
		EnhancedInput->BindAction(LeanLeftAction, ETriggerEvent::Started, this, &APlayerCharacter::StartLeanLeft);
		EnhancedInput->BindAction(LeanLeftAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopLeanLeft);

		EnhancedInput->BindAction(LeanRightAction, ETriggerEvent::Started, this, &APlayerCharacter::StartLeanRight);
		EnhancedInput->BindAction(LeanRightAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopLeanRight);
	}
}

void APlayerCharacter::Move(const FInputActionValue& InputValue)
{
	FVector2D InputVector = InputValue.Get<FVector2D>();

	if (IsValid(Controller))
	{
		// Get forward direction
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// Add movement Input
		AddMovementInput(ForwardDirection, InputVector.Y);
		AddMovementInput(RightDirection, InputVector.X);
	}
}

void APlayerCharacter::Look(const FInputActionValue& InputValue)
{
	FVector2D InputVector = InputValue.Get<FVector2D>();

	if (IsValid(Controller))
	{
		AddControllerYawInput(InputVector.X);
		AddControllerPitchInput(InputVector.Y);
	}
}

void APlayerCharacter::Jump()
{
	ACharacter::Jump();
}

void APlayerCharacter::CrouchUpdate(float Alpha)
{
	float NewHalfHeight = FMath::Lerp(DefaultCapsuleHalfHeight, CrouchingHalfHeight, Alpha);
	GetCapsuleComponent()->SetCapsuleHalfHeight(NewHalfHeight);

	// Calculate height difference
	float HeightDifference = DefaultCapsuleHalfHeight - NewHalfHeight;

	// Adjust actor location to prevent sinking into the ground
	float CameraZOffset = HeightDifference * CameraHeightMultiplier;

	FVector NewCameraLocation = DefaultCameraBloomLocation;
	NewCameraLocation.Z -= CameraZOffset;

	CameraBloom->SetRelativeLocation(NewCameraLocation);
}

void APlayerCharacter::CrouchFinished()
{
}

void APlayerCharacter::StartCrouch()
{
	bIsCrouching = true;
	CrouchingTimeline.Play();
	CharacterMovement->MaxWalkSpeed = CrouchMovementSpeed;
}

void APlayerCharacter::StopCrouch()
{
	if (CanStandUp())
	{
		bIsCrouching = false;
		CrouchingTimeline.Reverse();
		CharacterMovement->MaxWalkSpeed = DefaultMovementSpeed;
	}
}

void APlayerCharacter::ToggleCrouch()
{
	if (bIsCrouching)
	{
		StopCrouch();
	}
	else
	{
		StartCrouch();
	}
}

bool APlayerCharacter::CanStandUp()
{
	FVector CapsuleLocation = GetCapsuleComponent()->GetComponentLocation();

	FVector Start = CapsuleLocation + FVector(0.0f, 0.0f, 30.0f);
	FVector End = CapsuleLocation + FVector(0.0f, 0.0f, 90.0f);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	bool bHit = GetWorld()->SweepSingleByChannel(HitResult, Start, End, FQuat::Identity, TraceChannel, FCollisionShape::MakeSphere(TraceRadius), QueryParams);

	// Debug visualization 
	DrawDebugSphere(GetWorld(), Start, TraceRadius, 12, FColor::Green, false, 0.1f);
	DrawDebugSphere(GetWorld(), End, TraceRadius, 12, FColor::Blue, false, 0.1f);
	DrawDebugLine(GetWorld(), Start, End, bHit ? FColor::Red : FColor::Green, false, 0.1f);

	// If hit something, cannot stand up
	return !bHit;
}

void APlayerCharacter::LeanLeftUpdate(float Alpha)
{
	//Set Lean Value

	LeanValue = -Alpha;

	// Lerp Transform from Default to LeanLeft
	FTransform NewTransform;
	NewTransform.Blend(DefaultCameraTransform, LeanLeftTransform, Alpha);

	// Camera? Set Relative Transform
	Camera->SetRelativeTransform(NewTransform);
}

void APlayerCharacter::LeanLeftFinished()
{
}

void APlayerCharacter::LeanRightUpdate(float Alpha)
{
	//Set Lean Value

	LeanValue = -Alpha;

	// Lerp Transform from Default to LeanRight
	FTransform NewTransform;
	NewTransform.Blend(DefaultCameraTransform, LeanRightTransform, Alpha);

	// Camera? Set Relative Transform
	Camera->SetRelativeTransform(NewTransform);
}

void APlayerCharacter::LeanRightFinished()
{
}

void APlayerCharacter::StartLeanLeft()
{
	if (FMath::IsNearlyEqual(LeanValue, 0.0f))
	{
		LeanLeftTimeline.Play();
	}
}

void APlayerCharacter::StopLeanLeft()
{
	LeanLeftTimeline.Reverse();
}

void APlayerCharacter::StartLeanRight()
{
	if (FMath::IsNearlyEqual(LeanValue, 0.0f))
	{
		LeanRightTimeline.Play();
	}
}

void APlayerCharacter::StopLeanRight()
{
	LeanRightTimeline.Reverse();
}

