// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/EnemyAI.h"
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

	// Create Sight Config
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 1500.0f;
	SightConfig->LoseSightRadius = 1700.0f;
	SightConfig->PeripheralVisionAngleDegrees = 90.0f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

	// Configure AI Perception
	AIPerception->ConfigureSense(*SightConfig);
	AIPerception->SetDominantSense(SightConfig->GetSenseImplementation());

	// Bind Perception Update Event
	AIPerception->OnTargetPerceptionUpdated.AddDynamic(this, &AEnemyAI::OnTargetPerceptionUpdated);
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

		// Set Value as Bool - IsInvestigating
		bool bSuccessfullySensed = Stimulus.WasSuccessfullySensed();
		BlackboardComp->SetValueAsBool(FName("IsInvestigating"), bSuccessfullySensed);

		// Branch - Stimulus Successfully Sensed?
		if (bSuccessfullySensed)
		{
			// === True Branch ===

			// Get Player Character
			APawn* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);

			// Branch - SensedActor == PlayerCharacter?
			if (SensedActor == PlayerCharacter)
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
			// False Branch (SensedActor != PlayerCharacter)
			// → Do nothing
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
		// (추후 구현)
	}
}
