// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/EnemyAI.h"
#include "Character/PlayerCharacter.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AEnemyAI::AEnemyAI()
{
	// Create AI Perception Component
	AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
	if (!AIPerception)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create AIPerception component!"));
		return;
	}

	// Create Sight Config
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	if (SightConfig)
	{
		SightConfig->SightRadius = 1500.0f;
		SightConfig->LoseSightRadius = 1700.0f;
		SightConfig->PeripheralVisionAngleDegrees = 90.0f;
		SightConfig->DetectionByAffiliation.bDetectEnemies = true;
		SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
		SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create SightConfig!"));
	}

	// Create Hearing Config
	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	if (HearingConfig)
	{
		HearingConfig->HearingRange = 1500.0f;
		HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
		HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
		HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create HearingConfig!"));
	}

	// Configure AI Perception
	if (AIPerception && SightConfig && HearingConfig)
	{
		AIPerception->ConfigureSense(*SightConfig);
		AIPerception->ConfigureSense(*HearingConfig);
		AIPerception->SetDominantSense(SightConfig->GetSenseImplementation());

		// Bind Perception Update Event
		AIPerception->OnTargetPerceptionUpdated.AddDynamic(this, &AEnemyAI::OnTargetPerceptionUpdated);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to configure AI Perception!"));
	}
}

void AEnemyAI::BeginPlay()
{
	Super::BeginPlay();

	if (BehaviorTree)
	{
		RunBehaviorTree(BehaviorTree);
	}
}

void AEnemyAI::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	// Get Sense Class for Stimulus
	TSubclassOf<UAISense> SenseClass = UAIPerceptionSystem::GetSenseClassForStimulus(GetWorld(), Stimulus);

	// Handle Sense
	HandleSense(Actor, Stimulus);
}

void AEnemyAI::HandleSense(AActor* SensedActor, const FAIStimulus& Stimulus)
{
	if (!SensedActor)
	{
		return;
	}

	// Get Blackboard Component
	UBlackboardComponent* BlackboardComp = GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return;
	}

	// Get Sense Class
	TSubclassOf<UAISense> SenseClass = UAIPerceptionSystem::GetSenseClassForStimulus(GetWorld(), Stimulus);

	// Switch on String (Sense Class)
	if (SenseClass == UAISense_Sight::StaticClass())
	{
		// === AISense_Sight ===

		// Get successfully sensed flag
		bool bSuccessfullySensed = Stimulus.WasSuccessfullySensed();

		// Get Player Character
		APawn* PlayerPawn = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);

		// Check if sensed actor is player and apply stealth modifier
		if (SensedActor == PlayerPawn && bSuccessfullySensed && PlayerPawn)
		{
			// Try to cast to PlayerCharacter
			APlayerCharacter* Player = Cast<APlayerCharacter>(PlayerPawn);
			if (Player)
			{
				// Get stealth detection multiplier
				float DetectionMultiplier = Player->GetStealthDetectionMultiplier();

				// Get controlled pawn
				APawn* ControlledPawn = GetPawn();
				if (ControlledPawn)
				{
					// Calculate distance
					float Distance = FVector::Dist(ControlledPawn->GetActorLocation(), Player->GetActorLocation());

					// Get base sight radius (with null check)
					float BaseSightRadius = 1500.0f; // Default value
					if (SightConfig)
					{
						BaseSightRadius = SightConfig->SightRadius;
					}

					// Calculate modified sight radius based on stealth
					float ModifiedSightRadius = BaseSightRadius * DetectionMultiplier;

					// If player is too far considering stealth, don't detect
					if (Distance > ModifiedSightRadius)
					{
						bSuccessfullySensed = false;

						// Debug
						if (GEngine)
						{
							GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Cyan,
								FString::Printf(TEXT("Player too far: %.0f > %.0f (Stealth: %.1f%%)"),
									Distance, ModifiedSightRadius, DetectionMultiplier * 100.0f));
						}
					}
					else
					{
						// Debug
						if (GEngine)
						{
							GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Orange,
								FString::Printf(TEXT("Player DETECTED! Distance: %.0f / %.0f (Stealth: %.1f%%)"),
									Distance, ModifiedSightRadius, DetectionMultiplier * 100.0f));
						}
					}
				}
			}
		}

		// Set investigating state
		BlackboardComp->SetValueAsBool(FName("IsInvestigating"), bSuccessfullySensed);

		// Branch - Stimulus Successfully Sensed?
		if (bSuccessfullySensed)
		{
			// === True Branch ===

			// Branch - SensedActor == PlayerCharacter?
			if (SensedActor == PlayerPawn)
			{
				// === True Branch ===

				// Set Value as Object - TargetLocationActor
				BlackboardComp->SetValueAsObject(FName("TargetLocationActor"), SensedActor);

				// Get Controlled Pawn
				APawn* ControlledPawn = GetPawn();
				if (ControlledPawn)
				{
					// Cast To Character
					ACharacter* AsCharacter = Cast<ACharacter>(ControlledPawn);
					if (AsCharacter)
					{
						// Get Character Movement
						UCharacterMovementComponent* CharacterMovement = AsCharacter->GetCharacterMovement();
						if (CharacterMovement)
						{
							// Set Max Walk Speed = 400
							CharacterMovement->MaxWalkSpeed = 400.0f;
						}
					}
				}
			}
		}
		else
		{
			// === False Branch - Lost Sight ===

			// Set Value as Vector - TargetLocationVector
			FVector StimulusLocation = Stimulus.StimulusLocation;
			BlackboardComp->SetValueAsVector(FName("TargetLocationVector"), StimulusLocation);

			// Set Value as Object - TargetLocationActor (Clear)
			BlackboardComp->ClearValue(FName("TargetLocationActor"));
		}
	}
	else if (SenseClass == UAISense_Hearing::StaticClass())
	{
		// === AISense_Hearing ===

		// Get current investigation state
		bool bIsCurrentlyInvestigating = BlackboardComp->GetValueAsBool(FName("IsInvestigating"));

		// Set Value as Bool - IsInvestigating
		bool bSuccessfullySensed = Stimulus.WasSuccessfullySensed();
		BlackboardComp->SetValueAsBool(FName("IsInvestigating"), bSuccessfullySensed);

		// Set Value as Vector - TargetLocationVector
		FVector StimulusLocation = Stimulus.StimulusLocation;
		BlackboardComp->SetValueAsVector(FName("TargetLocationVector"), StimulusLocation);

		// Calculate distance to noise
		APawn* ControlledPawn = GetPawn();
		float DistanceToNoise = 0.0f;
		if (ControlledPawn)
		{
			DistanceToNoise = FVector::Dist(ControlledPawn->GetActorLocation(), StimulusLocation);
		}

		// Debug
		if (GEngine)
		{
			FColor DebugColor = bIsCurrentlyInvestigating ? FColor::Yellow : FColor::Orange;
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, DebugColor,
				FString::Printf(TEXT("NOISE DETECTED! Distance: %.0fcm | Location: (%.0f, %.0f, %.0f)"),
					DistanceToNoise, StimulusLocation.X, StimulusLocation.Y, StimulusLocation.Z));
		}

		// Log for debugging
		UE_LOG(LogTemp, Warning, TEXT("AISense_Hearing: Noise at (%.1f, %.1f, %.1f), Distance: %.1f"),
			StimulusLocation.X, StimulusLocation.Y, StimulusLocation.Z, DistanceToNoise);
	}
}