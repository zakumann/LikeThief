// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/PlayerCharacter.h"
#include "Character/LightDetector.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AIPerceptionSystem.h"
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
#include "Sound/SoundCue.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"

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

	// Sound defaults
	FootstepVolumeMultiplier = 1.0f;
	FootstepPitchMultiplier = 1.0f;
	LandingVolumeMultiplier = 1.0f;
	LandingPitchMultiplier = 1.0f;

	// Noise defaults
	LandingNoiseLoudness = 1.5f;
	bWasInAir = false;
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

	// AI perception
	UAIPerceptionSystem::RegisterPerceptionStimuliSource(this, UAISense_Sight::StaticClass(), this);
	UAIPerceptionSystem::RegisterPerceptionStimuliSource(this, UAISense_Hearing::StaticClass(), this);

	// Spawn Light Detector
	if (LightDetectorClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		LightDetector = GetWorld()->SpawnActor<ALightDetector>(LightDetectorClass, GetActorLocation(), FRotator::ZeroRotator, SpawnParams);

		if (LightDetector)
		{
			// Attach to character
			LightDetector->AttachToActor(this, FAttachmentTransformRules::SnapToTargetIncludingScale);
			UE_LOG(LogTemp, Log, TEXT("LightDetector spawned and attached"));
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

void APlayerCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	// Don't make landing noise during Mantle
	if (bIsMantling)
	{
		return;
	}

	// Only make noise if was actually in air (jumped or fell)
	if (bWasInAir)
	{
		// Reset flag
		bWasInAir = false;

		// Play landing sound
		PlayLandingSound();

		// Make landing noise for AI
		MakeLandingNoise();
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

	if (bIsMantling)
	{
		if (CheckMantleOverhead())
		{
			CancelMantle();
		}
	}

	// Track fall start height
	if (GetCharacterMovement()->IsFalling() && !bWasInAir)
	{
		FallStartZ = GetActorLocation().Z;
		bWasInAir = true;
	}

	// Update Stealth State
	UpdateStealthState();

	// Make Movement Noise
	MakeMovementNoise();
}

void APlayerCharacter::UpdateStealthState()
{
	if (!LightDetector)
	{
		CurrentStealthState = EStealthState::Exposed;
		CurrentBrightness = 1.0f;
		return;
	}

	// Get brightness from LightDetector
	CurrentBrightness = LightDetector->GetBrightness();

	// Clamp brightness to valid range
	CurrentBrightness = FMath::Clamp(CurrentBrightness, 0.0f, 1.0f);

	// Determine stealth state based on brightness
	EStealthState NewState;

	if (CurrentBrightness < FullyStealthThreshold)
	{
		NewState = EStealthState::FullyStealth;
	}
	else if (CurrentBrightness < PartiallyStealthThreshold)
	{
		NewState = EStealthState::PartiallyStealth;
	}
	else
	{
		NewState = EStealthState::Exposed;
	}

	// State change notification
	if (NewState != CurrentStealthState)
	{
		CurrentStealthState = NewState;

		// Debug
		if (GEngine)
		{
			FString StateName;
			FColor StateColor;

			switch (CurrentStealthState)
			{
			case EStealthState::FullyStealth:
				StateName = TEXT("FULLY STEALTH");
				StateColor = FColor::Green;
				break;
			case EStealthState::PartiallyStealth:
				StateName = TEXT("PARTIALLY STEALTH");
				StateColor = FColor::Yellow;
				break;
			case EStealthState::Exposed:
				StateName = TEXT("EXPOSED");
				StateColor = FColor::Red;
				break;
			default:
				StateName = TEXT("UNKNOWN");
				StateColor = FColor::White;
				break;
			}

			GEngine->AddOnScreenDebugMessage(100, 2.0f, StateColor,
				FString::Printf(TEXT("Stealth State: %s (Brightness: %.2f)"), *StateName, CurrentBrightness));
		}
	}
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

float APlayerCharacter::GetStealthDetectionMultiplier() const
{
	switch (CurrentStealthState)
	{
	case EStealthState::FullyStealth:
		return 0.2f; // AI Perception ability 20%
	case EStealthState::PartiallyStealth:
		return 0.6f; // AI Perception ability  60%
	case EStealthState::Exposed:
	default:
		return 1.0f; // AI Perception ability  100%
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
	// Step 1 : Camera Location Line Trace (Check WorldStatic)
	FVector CameraLocation = Camera->GetComponentLocation();
	FVector UpTraceStart = CameraLocation;
	FVector UpTraceEnd = CameraLocation + FVector(0.0f, 0.0f, MantleOverheadCheckHeight);

	FHitResult UpHitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	bool bUpHit = GetWorld()->LineTraceSingleByChannel(
		UpHitResult,
		UpTraceStart,
		UpTraceEnd,
		ECC_WorldStatic,
		QueryParams
	);

	// Step 2: No Crash upward, front Line Trace
	FVector CameraForward = Camera->GetForwardVector();
	FVector ForwardTraceStart = CameraLocation;
	FVector ForwardTraceEnd = CameraLocation + (CameraForward * 50.0f);

	FHitResult LineHitResult;
	bool bLineHit = GetWorld()->LineTraceSingleByChannel(
		LineHitResult,
		ForwardTraceStart,
		ForwardTraceEnd,
		ECC_Visibility,
		QueryParams
	);

	// Debug Forward Line Trace
	DrawDebugLine(GetWorld(), ForwardTraceStart, ForwardTraceEnd, bLineHit ? FColor::Green : FColor::Red, false, 0.1f);

	if (bLineHit)
	{
		// Step 3: Ready Sphere Trace
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
			// Branch True: If crash, Mantle is impossible
			bHitDetected = false;
		}
		else
		{
			// Branch False: If no crash, Can Mantle
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

bool APlayerCharacter::CheckMantleOverhead()
{
	FVector ActorLocation = GetActorLocation();
	FVector TraceStart = ActorLocation;
	FVector TraceEnd = ActorLocation + FVector(0.0f, 0.0f, MantleOverheadCheckHeight);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	bool bHit = GetWorld()->SweepSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		FQuat::Identity,
		ECC_WorldStatic,
		FCollisionShape::MakeSphere(MantleOverheadCheckRadius),
		QueryParams
	);

	// Debug visualization
	DrawDebugSphere(GetWorld(), TraceStart, MantleOverheadCheckRadius, 12, FColor::Cyan, false, 0.1f);
	DrawDebugSphere(GetWorld(), TraceEnd, MantleOverheadCheckRadius, 12, bHit ? FColor::Red : FColor::Green, false, 0.1f);
	DrawDebugLine(GetWorld(), TraceStart, TraceEnd, bHit ? FColor::Red : FColor::Green, false, 0.1f);

	return bHit;
}

void APlayerCharacter::MantleUp()
{ 
	// Set HitDetected is false
	bHitDetected = false;

	bIsMantling = true;

	// Timeline play from start
	MantleTimeline.PlayFromStart();
}

void APlayerCharacter::MakeMovementNoise()
{
	// Check if should make noise
	if (!ShouldMakeNoise())
	{
		return;
	}

	// Check time interval
	float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastNoiseTime < MovementNoiseInterval)
	{
		return;
	}

	// Update last noise time
	LastNoiseTime = CurrentTime;

	// Get noise loudness
	float NoiseLoudness = GetCurrentNoiseLoudness();

	// Play Footstep Sound
	PlayFootstepSound();

	// Report noise event
	UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetActorLocation(), NoiseLoudness, this, NoiseRange, FName("Footstep"));

	// Debug
	if (GEngine)
	{
		FColor DebugColor = bIsCrouching ? FColor::Green : FColor::Yellow;
		GEngine->AddOnScreenDebugMessage(-1, 0.5f, DebugColor,
			FString::Printf(TEXT("Footstep Noise: %.2f"), NoiseLoudness));
	}
}

bool APlayerCharacter::ShouldMakeNoise() const
{
	// Don't make noise when crouching
	if (bIsCrouching)
	{
		return false;
	}

	// Don't make noise when mantling
	if (bIsMantling)
	{
		return false;
	}

	// Don't make noise when not moving
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.0f; // Ignore vertical velocity
	float Speed = Velocity.Size();

	if (Speed < 10.0f) // Minimum speed threshold
	{
		return false;
	}

	// Don't make noise when falling
	if (GetCharacterMovement()->IsFalling())
	{
		return false;
	}

	return true;
}

float APlayerCharacter::GetCurrentNoiseLoudness() const
{
	// Get horizontal velocity
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.0f;
	float Speed = Velocity.Size();

	// Get max walk speed 
	float MaxWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;

	// Calculate loudness based on speed
	float SpeedRatio = FMath::Clamp(Speed / MaxWalkSpeed, 0.0f, 1.0f);

	// Lerp between walk and run loudness
	float NoiseLoudness = FMath::Lerp(WalkNoiseLoudness, RunNoiseLoudness, SpeedRatio);

	return NoiseLoudness;
}

void APlayerCharacter::PlayFootstepSound()
{
	// Check if FootstepSoundCue is assigned
	if (!FootstepSoundCue)
	{
		return;
	}

	// Get Current speed for dynamic volume/pitch
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.0f;
	float Speed = Velocity.Size();
	float MaxWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	float SpeedRatio = FMath::Clamp(Speed / MaxWalkSpeed, 0.0f, 1.0f);

	// Calculate volume and pitch based on speed
	float Volume = FMath::Lerp(0.5f, 1.0f, SpeedRatio) * FootstepVolumeMultiplier;
	float Pitch = FMath::Lerp(0.9f, 1.1f, SpeedRatio) * FootstepPitchMultiplier;

	// Play sound at player location
	UGameplayStatics::PlaySoundAtLocation(GetWorld(), FootstepSoundCue, GetActorLocation(), Volume, Pitch);
}

void APlayerCharacter::PlayLandingSound()
{
	// Use LandingSoundCue if available, otherwise use FootstepSoundCue
	USoundCue* SoundToPlay = LandingSoundCue ? LandingSoundCue : FootstepSoundCue;

	if (!SoundToPlay)
	{
		return;
	}

	// Dynamic volume and pitch based on fall intensity
	float Volume = 1.0f * LandingVolumeMultiplier;
	float Pitch = 1.0f * LandingPitchMultiplier;

	// Play sound at player location
	UGameplayStatics::PlaySoundAtLocation(
		GetWorld(),
		SoundToPlay,
		GetActorLocation(),
		Volume,
		Pitch
	);

	// Debug
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Cyan,
			TEXT("Landing Sound Played"));
	}
}

void APlayerCharacter::MakeLandingNoise()
{
	// Report noise event for AI
	UAISense_Hearing::ReportNoiseEvent(
		GetWorld(),
		GetActorLocation(),
		LandingNoiseLoudness,
		this,
		NoiseRange,
		FName("Landing")
	);

	// Debug
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red,
			FString::Printf(TEXT("Landing Noise: %.2f"), LandingNoiseLoudness));
	}
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
	bIsMantling = false;
}

void APlayerCharacter::CancelMantle()
{
	// Cancel Mantle
	bIsMantling = false;
	bHitDetected = false;
	bHold = false;

	// Stop Timeline
	MantleTimeline.Stop();

	//Clear Timer
	GetWorld()->GetTimerManager().ClearTimer(MantleCheckTimerHandle);

	UE_LOG(LogTemp, Warning, TEXT("Mantle Cancelled: WorldStatic detected overhead"));
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

