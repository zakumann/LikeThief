// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/PlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AIPerceptionComponent.h"
#include "Character/LightDetector.h"

APlayerCharacter::APlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create Spring Arm
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 300.0f;
	SpringArm->bUsePawnControlRotation = true;

	// Create Camera
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;

	// Configure Character Movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = 600.0f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.0f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.0f;

	// Don't rotate character to camera direction
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Initialize Stealth State
	CurrentStealthState = EStealthState::Exposed;
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	// Spawn LightDetector
	if (LightDetectorClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		LightDetector = GetWorld()->SpawnActor<AActor>(LightDetectorClass, GetActorLocation(), GetActorRotation(), SpawnParams);

		if (LightDetector)
		{
			LightDetector->AttachToActor(this, FAttachmentTransformRules::SnapToTargetIncludingScale);
		}
	}

	// Register for AI Perception
	UAIPerceptionSystem::RegisterPerceptionStimuliSource(this, UAISense_Sight::StaticClass(), this);
	UAIPerceptionSystem::RegisterPerceptionStimuliSource(this, UAISense_Hearing::StaticClass(), this);
}

void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update Crouch
	UpdateCrouch(DeltaTime);

	// Update Lean
	UpdateLean(DeltaTime);

	// Update Stealth State
	UpdateStealthState();

	// Update Light Value
	if (LightDetector)
	{
		ALightDetector* Detector = Cast<ALightDetector>(LightDetector);
		if (Detector)
		{
			float Brightness = Detector->CalculateBrightness();
			// Normalize (0-255 → 0-1)
			CurrentLightValue = FMath::Clamp(Brightness / 255.0f, 0.0f, 1.0f);
		}
	}

	// Handle Jump Hold for Mantle
	if (bIsHoldingJump)
	{
		JumpHoldTime += DeltaTime;

		if (JumpHoldTime >= JumpHoldThreshold && !bIsMantling)
		{
			CheckMantle();
		}
	}

	// Make Movement Noise
	MakeMovementNoise();

	// Track if in air
	bWasInAir = GetCharacterMovement()->IsFalling();
}

void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Move
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Move);

		// Look
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Look);

		// Jump
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &APlayerCharacter::StartJump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopJump);

		// Crouch
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &APlayerCharacter::StartCrouch);
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopCrouch);

		// Lean
		EnhancedInputComponent->BindAction(LeanLeftAction, ETriggerEvent::Started, this, &APlayerCharacter::StartLeanLeft);
		EnhancedInputComponent->BindAction(LeanLeftAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopLeanLeft);
		EnhancedInputComponent->BindAction(LeanRightAction, ETriggerEvent::Started, this, &APlayerCharacter::StartLeanRight);
		EnhancedInputComponent->BindAction(LeanRightAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopLeanRight);
	}
}

void APlayerCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	// Reset mantle state
	bIsMantling = false;
	JumpHoldTime = 0.0f;

	// Make landing noise
	if (!bIsMantling && bWasInAir)
	{
		PlayLandingSound();
		MakeLandingNoise();
	}

	bWasInAir = false;
}

// === Movement Functions ===

void APlayerCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void APlayerCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void APlayerCharacter::StartJump()
{
	bIsHoldingJump = true;
	JumpHoldTime = 0.0f;
	Jump();
}

void APlayerCharacter::StopJump()
{
	bIsHoldingJump = false;
	JumpHoldTime = 0.0f;
	StopJumping();
}

void APlayerCharacter::StartCrouch()
{
	if (!bIsCrouching && !bIsCrouchingInterpolating)
	{
		bIsCrouching = true;
		bIsCrouchingInterpolating = true;
		CrouchInterpolationTime = 0.0f;
	}
}

void APlayerCharacter::StopCrouch()
{
	if (bIsCrouching && !bIsCrouchingInterpolating)
	{
		bIsCrouching = false;
		bIsCrouchingInterpolating = true;
		CrouchInterpolationTime = 0.0f;
	}
}

void APlayerCharacter::UpdateCrouch(float DeltaTime)
{
	if (bIsCrouchingInterpolating)
	{
		CrouchInterpolationTime += DeltaTime * CrouchSpeed;

		float TargetHeight = bIsCrouching ? CrouchingCapsuleHalfHeight : StandingCapsuleHalfHeight;
		float CurrentHeight = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
		float NewHeight = FMath::Lerp(CurrentHeight, TargetHeight, CrouchInterpolationTime);

		GetCapsuleComponent()->SetCapsuleHalfHeight(NewHeight);

		if (FMath::IsNearlyEqual(NewHeight, TargetHeight, 1.0f))
		{
			GetCapsuleComponent()->SetCapsuleHalfHeight(TargetHeight);
			bIsCrouchingInterpolating = false;
		}
	}
}

void APlayerCharacter::StartLeanLeft()
{
	if (!bIsLeaningLeft && !bIsLeaningRight && !bIsLeaningInterpolating)
	{
		bIsLeaningLeft = true;
		bIsLeaningInterpolating = true;
		LeanInterpolationTime = 0.0f;
		TargetLeanRotation = FRotator(0.0f, 0.0f, LeanAngle);
		TargetLeanLocation = FVector(0.0f, -LeanDistance, 0.0f);
	}
}

void APlayerCharacter::StopLeanLeft()
{
	if (bIsLeaningLeft && !bIsLeaningInterpolating)
	{
		bIsLeaningLeft = false;
		bIsLeaningInterpolating = true;
		LeanInterpolationTime = 0.0f;
		TargetLeanRotation = FRotator::ZeroRotator;
		TargetLeanLocation = FVector::ZeroVector;
	}
}

void APlayerCharacter::StartLeanRight()
{
	if (!bIsLeaningRight && !bIsLeaningLeft && !bIsLeaningInterpolating)
	{
		bIsLeaningRight = true;
		bIsLeaningInterpolating = true;
		LeanInterpolationTime = 0.0f;
		TargetLeanRotation = FRotator(0.0f, 0.0f, -LeanAngle);
		TargetLeanLocation = FVector(0.0f, LeanDistance, 0.0f);
	}
}

void APlayerCharacter::StopLeanRight()
{
	if (bIsLeaningRight && !bIsLeaningInterpolating)
	{
		bIsLeaningRight = false;
		bIsLeaningInterpolating = true;
		LeanInterpolationTime = 0.0f;
		TargetLeanRotation = FRotator::ZeroRotator;
		TargetLeanLocation = FVector::ZeroVector;
	}
}

void APlayerCharacter::UpdateLean(float DeltaTime)
{
	if (bIsLeaningInterpolating)
	{
		LeanInterpolationTime += DeltaTime * LeanSpeed;

		FRotator CurrentRotation = Camera->GetRelativeRotation();
		FVector CurrentLocation = Camera->GetRelativeLocation();

		FRotator NewRotation = FMath::Lerp(CurrentRotation, TargetLeanRotation, LeanInterpolationTime);
		FVector NewLocation = FMath::Lerp(CurrentLocation, TargetLeanLocation, LeanInterpolationTime);

		Camera->SetRelativeRotation(NewRotation);
		Camera->SetRelativeLocation(NewLocation);

		if (LeanInterpolationTime >= 1.0f)
		{
			Camera->SetRelativeRotation(TargetLeanRotation);
			Camera->SetRelativeLocation(TargetLeanLocation);
			bIsLeaningInterpolating = false;
		}
	}
}

// === Mantle System ===

void APlayerCharacter::CheckMantle()
{
	FVector Start = GetActorLocation();
	FVector ForwardVector = GetActorForwardVector();
	FVector End = Start + (ForwardVector * MantleCheckDistance);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_WorldStatic, QueryParams))
	{
		FVector MantleLocation = HitResult.Location + FVector(0, 0, MantleHeight);

		FVector OverheadStart = MantleLocation;
		FVector OverheadEnd = MantleLocation + FVector(0, 0, 100);

		if (!GetWorld()->LineTraceSingleByChannel(HitResult, OverheadStart, OverheadEnd, ECC_WorldStatic, QueryParams))
		{
			PerformMantle(MantleLocation);
		}
	}
}

void APlayerCharacter::PerformMantle(FVector MantleLocation)
{
	bIsMantling = true;
	SetActorLocation(MantleLocation);
	GetCharacterMovement()->Velocity = FVector::ZeroVector;
}

// === Stealth System ===

float APlayerCharacter::GetStealthDetectionMultiplier() const
{
	switch (CurrentStealthState)
	{
	case EStealthState::FullyStealth:
		return 0.2f;
	case EStealthState::PartiallyStealth:
		return 0.6f;
	case EStealthState::Exposed:
		return 1.0f;
	default:
		return 1.0f;
	}
}

float APlayerCharacter::GetLightValue() const
{
	return CurrentLightValue;
}

void APlayerCharacter::UpdateStealthState()
{
	if (!LightDetector)
	{
		CurrentStealthState = EStealthState::Exposed;
		return;
	}

	ALightDetector* Detector = Cast<ALightDetector>(LightDetector);
	if (!Detector)
	{
		CurrentStealthState = EStealthState::Exposed;
		return;
	}

	float Brightness = Detector->CalculateBrightness();
	float NormalizedBrightness = Brightness / 255.0f;

	if (NormalizedBrightness <= FullyStealthThreshold)
	{
		CurrentStealthState = EStealthState::FullyStealth;
	}
	else if (NormalizedBrightness <= PartiallyStealthThreshold)
	{
		CurrentStealthState = EStealthState::PartiallyStealth;
	}
	else
	{
		CurrentStealthState = EStealthState::Exposed;
	}
}

// === Noise System ===

void APlayerCharacter::MakeMovementNoise()
{
	TimeSinceLastFootstep += GetWorld()->GetDeltaSeconds();

	if (ShouldMakeNoise() && TimeSinceLastFootstep >= FootstepNoiseInterval)
	{
		float Loudness = GetCurrentNoiseLoudness();

		UAISense_Hearing::ReportNoiseEvent(
			GetWorld(),
			GetActorLocation(),
			Loudness,
			this,
			0.0f,
			FName("Footstep")
		);

		PlayFootstepSound();
		TimeSinceLastFootstep = 0.0f;
	}
}

bool APlayerCharacter::ShouldMakeNoise() const
{
	if (bIsCrouching || bIsMantling || GetCharacterMovement()->IsFalling())
	{
		return false;
	}

	FVector Velocity = GetVelocity();
	Velocity.Z = 0.0f;
	float Speed = Velocity.Size();

	return Speed > 10.0f;
}

float APlayerCharacter::GetCurrentNoiseLoudness() const
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.0f;
	float Speed = Velocity.Size();
	float MaxSpeed = GetCharacterMovement()->MaxWalkSpeed;

	float SpeedRatio = FMath::Clamp(Speed / MaxSpeed, 0.0f, 1.0f);

	return FMath::Lerp(WalkNoiseLoudness, RunNoiseLoudness, SpeedRatio);
}

void APlayerCharacter::PlayFootstepSound()
{
	if (FootstepSound)
	{
		FVector Velocity = GetVelocity();
		Velocity.Z = 0.0f;
		float Speed = Velocity.Size();
		float MaxSpeed = GetCharacterMovement()->MaxWalkSpeed;
		float SpeedRatio = FMath::Clamp(Speed / MaxSpeed, 0.0f, 1.0f);

		float Volume = FMath::Lerp(0.5f, 1.0f, SpeedRatio);
		float Pitch = FMath::Lerp(0.9f, 1.1f, SpeedRatio);

		UGameplayStatics::PlaySoundAtLocation(
			this,
			FootstepSound,
			GetActorLocation(),
			Volume,
			Pitch
		);
	}
}

void APlayerCharacter::MakeLandingNoise()
{
	UAISense_Hearing::ReportNoiseEvent(
		GetWorld(),
		GetActorLocation(),
		LandingNoiseLoudness,
		this,
		0.0f,
		FName("Landing")
	);
}

void APlayerCharacter::PlayLandingSound()
{
	if (LandingSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			LandingSound,
			GetActorLocation(),
			1.0f,
			1.0f
		);
	}
}