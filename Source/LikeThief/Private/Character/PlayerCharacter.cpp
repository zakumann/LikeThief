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
#include "TimerManager.h"

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

	// Setup Mantle Timeline(T_Mantle)
	if (MantleCurve)
	{
		FOnTimelineFloat MantleProgressUpdate;
		MantleProgressUpdate.BindUFunction(this, FName("MantleUpdate"));

		FOnTimelineEvent MantleFinishedEvent;
		MantleFinishedEvent.BindUFunction(this, FName("MantleFinished"));

		MantleTimeline.AddInterpFloat(MantleCurve, MantleProgressUpdate);
		MantleTimeline.SetTimelineFinishedFunc(MantleFinishedEvent);
	}
}

// Called every frame
void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CrouchingTimeline.TickTimeline(DeltaTime);
	LeanLeftTimeline.TickTimeline(DeltaTime);
	LeanRightTimeline.TickTimeline(DeltaTime);
	MantleTimeline.TickTimeline(DeltaTime);
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
		EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopJump);
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
	// IsFalling?
	if (!CharacterMovement->IsFalling())
	{
		// False: jump
		ACharacter::Jump();
	}
	else
	{
		//True: bHold is true
		bHold = true;

		// bHold && IsFalling?
		CheckMantleCondition();
	}
}

void APlayerCharacter::StopJump()
{
	bHold = false;
	ACharacter::StopJumping();
}

void APlayerCharacter::CheckMantleCondition()
{
	//if bHoldis true and IsFalling also true
	if (bHold && CharacterMovement->IsFalling())
	{
		if (bHitDetected)
		{
			MantleUp();
		}
		else
		{
			MantleCheck();

			// After Delay 0.001, back to CheckMantleCondition
			GetWorld()->GetTimerManager().SetTimer(
				MantleCheckTimerHandle,
				this,
				&APlayerCharacter::CheckMantleCondition,
				MantleCheckDelay,
				false
			);
		}
	}
}
void APlayerCharacter::MantleCheck()
{
	// Line Trace through Camera location
	FVector CameraLocation = Camera->GetComponentLocation();
	FVector CameraForward = Camera->GetForwardVector();
	FVector TraceStart = CameraLocation;
	FVector TraceEnd = CameraLocation + (CameraForward * 50.0f);

	FHitResult LineHitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	bool bLineHit = GetWorld()->LineTraceSingleByChannel(
		LineHitResult,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		QueryParams
	);

	// Deebug Line Trace
	DrawDebugLine(GetWorld(), TraceStart, TraceEnd, bLineHit ? FColor::Green : FColor::Red, false, 0.1f);

	if (bLineHit)
	{
		// Ready Sphere Trace
		FVector LineHitLocation = LineHitResult.Location;
		FVector ForwardOffset = CameraForward * MantleForwardDistance;
		FVector SphereStart = LineHitLocation + ForwardOffset;
		SphereStart.Z += 100.0f;

		FVector SphereEnd = SphereStart;
		SphereEnd.Z += 96.0f;

		FHitResult SphereHitResult;
		bool bSphereHit = GetWorld()->SweepSingleByChannel(
			SphereHitResult,
			SphereStart,
			SphereEnd,
			FQuat::Identity,
			ECC_Visibility,
			FCollisionShape::MakeSphere(MantleSphereRadius),
			QueryParams
		);

		// Debug Sphere Trace
		DrawDebugSphere(GetWorld(), SphereStart, MantleSphereRadius, 12, FColor::Yellow, false, 0.1f);
		DrawDebugSphere(GetWorld(), SphereEnd, MantleSphereRadius, 12, bSphereHit ? FColor::Red : FColor::Green, false, 0.1f);
		DrawDebugLine(GetWorld(), SphereStart, SphereEnd, bSphereHit ? FColor::Red : FColor::Green, false, 0.1f);

		if (bSphereHit)
		{
			// if there is collision mantle is deactivate
			bHitDetected = false;
		}
		else
		{
			// Without any collision mantle is activate
			bHitDetected = true;
			MantleTargetLocation = LineHitLocation + ForwardOffset;
			MantleTargetLocation.Z += 100.0f;
		}
	}
	else
	{
		bHitDetected = false;
	}
}
void APlayerCharacter::MantleUp()
{ 
	// Set HitDetected is false
	bHitDetected = false;

	// Timeline play from start
	MantleTimeline.PlayFromStart();
}

void APlayerCharacter::MantleUpdate(float Alpha)
{
	// Lerp Actor Location
	FVector CurrentLocation = GetActorLocation();
	FVector NewLocation = FMath::Lerp(CurrentLocation, MantleTargetLocation, Alpha);

	SetActorLocation(NewLocation);
}

void APlayerCharacter::MantleFinished()
{
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

