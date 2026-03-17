// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/EnemyAI.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Character/PlayerCharacter.h"

AEnemyAI::AEnemyAI()
{
	// Create AI Perception Component
	AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));

	// Create Sight Config
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 1000.0f;
	SightConfig->LoseSightRadius = 1500.0f;
	SightConfig->PeripheralVisionAngleDegrees = 50.0f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

	// Create Hearing Config
	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->HearingRange = 1500.0f;
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;

	// Configure AI Perception
	AIPerception->ConfigureSense(*SightConfig);
	AIPerception->ConfigureSense(*HearingConfig);
	AIPerception->SetDominantSense(SightConfig->GetSenseImplementation());

	// Bind Perception Update Event
	AIPerception->OnTargetPerceptionUpdated.AddDynamic(this, &AEnemyAI::OnTargetPerceptionUpdated);

	// Detection Settings
	ProximityDetectionRange = 50.0f;
	ProximityDetectionAngle = 90.0f;
	LightThreshold = 0.5f;
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

	// === Get Class Display Name ===
	FString SenseClassName = SenseClass ? SenseClass->GetName() : TEXT("None");

	// Handle Sense
	HandleSense(SenseClassName, Actor, Stimulus);
}

void AEnemyAI::HandleSense(FString Selection, AActor* SensedActor, const FAIStimulus& Stimulus)
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

	// === Switch on String (Selection) ===
	if (Selection.Contains(TEXT("Sight")))
	{
		// ============================================
		// === AISense_Sight ===
		// ============================================

		// Get successfully sensed flag
		bool bSuccessfullySensed = Stimulus.WasSuccessfullySensed();

		// Get Player Character
		APawn* PlayerPawn = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);

		// === Light Detection Check (Only if player is sensed) ===
		if (SensedActor == PlayerPawn && bSuccessfullySensed && PlayerPawn)
		{
			// Cast to PlayerCharacter
			APlayerCharacter* Player = Cast<APlayerCharacter>(PlayerPawn);
			if (Player)
			{
				// Get controlled pawn
				APawn* ControlledPawn = GetPawn();
				if (ControlledPawn)
				{
					// Calculate distance to player
					float Distance = FVector::Dist(ControlledPawn->GetActorLocation(), Player->GetActorLocation());

					// === Get Light Value from Player ===
					float LightValue = Player->GetLightValue();

					// Debug Light Value
					if (GEngine)
					{
						GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::White,
							FString::Printf(TEXT("Player Light Value: %.2f"), LightValue));
					}

					// === Check 1: Light Threshold (최우선 체크) ===
					if (LightValue < LightThreshold)
					{
						// 너무 어두움 - 무조건 감지 실패
						bSuccessfullySensed = false;

						// Debug
						if (GEngine)
						{
							GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Green,
								FString::Printf(TEXT("TOO DARK! Light: %.2f < %.2f (Passing through)"),
									LightValue, LightThreshold));
						}
					}
					else
					{
						// 충분히 밝음 - 추가 체크 진행

						// === Check 2: Proximity Detection (전방 근접) ===
						if (Distance <= ProximityDetectionRange)
						{
							// 근접 거리 - 방향 체크

							// Enemy의 전방 방향 벡터
							FVector EnemyForward = ControlledPawn->GetActorForwardVector();
							EnemyForward.Z = 0.0f;
							EnemyForward.Normalize();

							// Enemy → Player 방향 벡터
							FVector ToPlayer = Player->GetActorLocation() - ControlledPawn->GetActorLocation();
							ToPlayer.Z = 0.0f;
							ToPlayer.Normalize();

							// 내적(Dot Product)으로 각도 계산
							float DotProduct = FVector::DotProduct(EnemyForward, ToPlayer);
							float AngleRadians = FMath::Acos(DotProduct);
							float AngleDegrees = FMath::RadiansToDegrees(AngleRadians);

							// 전방 각도 범위 내인지 체크
							if (AngleDegrees <= ProximityDetectionAngle)
							{
								// 전방 근접 - 발각!
								bSuccessfullySensed = true;

								// Debug
								if (GEngine)
								{
									GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red,
										FString::Printf(TEXT("TOO CLOSE IN FRONT! Detected at %.0fcm, Angle: %.0f°"),
											Distance, AngleDegrees));
								}
							}
							else
							{
								// 뒤나 옆 - 은신 규칙 적용
								float StealthMultiplier = Player->GetStealthDetectionMultiplier();
								float BaseSightRadius = SightConfig ? SightConfig->SightRadius : 1500.0f;
								float ModifiedSightRadius = BaseSightRadius * StealthMultiplier;

								if (Distance > ModifiedSightRadius)
								{
									bSuccessfullySensed = false;
								}

								// Debug
								if (GEngine)
								{
									GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Cyan,
										FString::Printf(TEXT("Close but behind/side: %.0fcm, Angle: %.0f° (Safe)"),
											Distance, AngleDegrees));
								}
							}
						}
						else
						{
							// === Check 3: Normal Distance - Stealth Detection ===
							float StealthMultiplier = Player->GetStealthDetectionMultiplier();
							float BaseSightRadius = SightConfig ? SightConfig->SightRadius : 1500.0f;
							float ModifiedSightRadius = BaseSightRadius * StealthMultiplier;

							// 거리 체크
							if (Distance > ModifiedSightRadius)
							{
								bSuccessfullySensed = false;

								// Debug
								if (GEngine)
								{
									GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Cyan,
										FString::Printf(TEXT("Player too far: %.0f > %.0f (Stealth: %.1f%%)"),
											Distance, ModifiedSightRadius, StealthMultiplier * 100.0f));
								}
							}
							else
							{
								// 발각!
								bSuccessfullySensed = true;

								// Debug
								if (GEngine)
								{
									GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Orange,
										FString::Printf(TEXT("Player DETECTED! Distance: %.0f, Light: %.2f"),
											Distance, LightValue));
								}
							}
						}
					}
				}
			}
		}

		// === Set Blackboard Value as Bool: IsInvestigating ===
		BlackboardComp->SetValueAsBool(FName("IsInvestigating"), bSuccessfullySensed);

		// === Branch: Stimulus Successfully Sensed? ===
		if (bSuccessfullySensed)
		{
			// === True Branch - Player Detected ===

			// Branch - SensedActor == PlayerCharacter?
			if (SensedActor == PlayerPawn)
			{
				// === Set Value as Object: TargetLocationActor ===
				BlackboardComp->SetValueAsObject(FName("TargetLocationActor"), SensedActor);

				// === Get Controlled Pawn ===
				APawn* ControlledPawn = GetPawn();
				if (ControlledPawn)
				{
					// === Cast To Character ===
					ACharacter* AsCharacter = Cast<ACharacter>(ControlledPawn);
					if (AsCharacter)
					{
						// === Get Character Movement ===
						UCharacterMovementComponent* CharacterMovement = AsCharacter->GetCharacterMovement();
						if (CharacterMovement)
						{
							// === Set Max Walk Speed = 400 ===
							CharacterMovement->MaxWalkSpeed = 400.0f;
						}
					}
				}
			}
		}
		else
		{
			// === False Branch - Lost Sight ===

			// Get Stimulus Location
			FVector StimulusLocation = Stimulus.StimulusLocation;

			// === Set Value as Vector: TargetLocationVector ===
			BlackboardComp->SetValueAsVector(FName("TargetLocationVector"), StimulusLocation);

			// === Set Value as Object: Clear TargetLocationActor ===
			BlackboardComp->ClearValue(FName("TargetLocationActor"));
		}
	}
	else if (Selection.Contains(TEXT("Hearing")))
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
