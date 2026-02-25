// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/EnemyAI.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"

AEnemyAI::AEnemyAI()
{
	// Create AI Perception Component
	AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));

	// Create Sight Config
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 1000.0f;
	SightConfig->LoseSightRadius = 1800.0f;
	SightConfig->PeripheralVisionAngleDegrees = 75.0f;
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
	// Get Scene Class for Stimulus
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

	UBlackboardComponent* BlackboardComp = GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return;
	}

	// Get Sense Class
	TSubclassOf<UAISense> SenseClass = UAIPerceptionSystem::GetSenseClassForStimulus(GetWorld(), Stimulus);

	// Switch on Sense Class
	if (SenseClass == UAISense_Sight::StaticClass())
	{
		// AISense_Sight

		// Set Value as Bool - IsInvestigating
		bool bSuccessfullySensed = Stimulus.WasSuccessfullySensed();
		BlackboardComp->SetValueAsBool(FName("IsInvestigating"), bSuccessfullySensed);

		// Branch
		if (bSuccessfullySensed)
		{
			// Set Value as Object - TargetLocationActor
			BlackboardComp->SetValueAsObject(FName("TargetLocationActor"), SensedActor);
		}
		else
		{
			// Lost sight - clear target
			BlackboardComp->ClearValue(FName("TargetLocationActor"));
		}
	}
	else if (SenseClass == UAISense_Hearing::StaticClass())
	{
		// AISense_Hearing (later)
		// Current empty
	}
}
