// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/BTTask_FindRandomPoint.h"
#include "Character/Enemy/EnemyBase.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

UBTTask_FindRandomPoint::UBTTask_FindRandomPoint()
{
	NodeName = "Find Random Point";
	WalkSpeed = 200.0f;
	SearchRadiusMax = 1000.0f;
}

EBTNodeResult::Type UBTTask_FindRandomPoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// Get AI Controller
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		return EBTNodeResult::Failed;
	}

	// Get Controlled Pawn
	APawn* ControlledPawn = AIController->GetPawn();
	if (!ControlledPawn)
	{
		return EBTNodeResult::Failed;
	}

	// Cast To EnemyBase
	AEnemyBase* EnemyBase = Cast<AEnemyBase>(ControlledPawn);
	if (!EnemyBase)
	{
		return EBTNodeResult::Failed;
	}

	// Get Character Movement
	UCharacterMovementComponent* CharacterMovement = EnemyBase->GetCharacterMovement();
	if (!CharacterMovement)
	{
		return EBTNodeResult::Failed;
	}

	// Set Max Walk Speed
	CharacterMovement->MaxWalkSpeed = WalkSpeed;

	// Get Actor Location
	FVector ActorLocation = ControlledPawn->GetActorLocation();

	// Get Navigation System
	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!NavSys)
	{
		return EBTNodeResult::Failed;
	}

	// Get Random Reachable Point in Radius
	FNavLocation RandomLocation;
	bool bSuccess = NavSys->GetRandomReachablePointInRadius(ActorLocation, SearchRadiusMax, RandomLocation);

	if (!bSuccess)
	{
		return EBTNodeResult::Failed;
	}

	// Get Blackboard Component
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return EBTNodeResult::Failed;
	}

	// Set Blackboard Value as Vector
	BlackboardComp->SetValueAsVector(TargetLocation.SelectedKeyName, RandomLocation.Location);

	// Finish Execute - Success
	return EBTNodeResult::Succeeded;
}
