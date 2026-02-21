// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/BTTask_FindPathPoint.h"
#include "Character/Enemy/EnemyGuard.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_FindPathPoint::UBTTask_FindPathPoint()
{
	NodeName = "Find Path Point";
}

EBTNodeResult::Type UBTTask_FindPathPoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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

	// Cast To BP_EnemyGuard
	AEnemyGuard* EnemyGuardPawn = Cast<AEnemyGuard>(ControlledPawn);
	if (!EnemyGuardPawn)
	{
		return EBTNodeResult::Failed;
	}

	// Check if PatrolPath is valid
	if (EnemyGuardPawn->PatrolPath.Num() == 0)
	{
		return EBTNodeResult::Failed;
	}

	// Get Current Patrol Point
	int32 CurrentPathIndex = EnemyGuardPawn->CurrentPathIndex;
	if (!EnemyGuardPawn->PatrolPath.IsValidIndex(CurrentPathIndex))
	{
		return EBTNodeResult::Failed;
	}

	APathPoint* CurrentPatrolPoint = EnemyGuardPawn->PatrolPath[CurrentPathIndex];
	if (!CurrentPatrolPoint)
	{
		return EBTNodeResult::Failed;
	}

	// Get Blackboard Component
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return EBTNodeResult::Failed;
	}

	// Get Actor Location
	FVector PatrolPointLocation = CurrentPatrolPoint->GetActorLocation();

	// Get Random Reachable Point in Radius
	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	FNavLocation RandomLocation;

	if (NavSys && NavSys->GetRandomReachablePointInRadius(PatrolPointLocation, 20.0f, RandomLocation))
	{
		// Set Blackboard Value as Vector
		BlackboardComp->SetValueAsVector(BB_TargetVector.SelectedKeyName, RandomLocation.Location);
	}
	else
	{
		// Fallbakc to exact location
		BlackboardComp->SetValueAsVector(BB_TargetVector.SelectedKeyName, PatrolPointLocation);
	}

	// Calculate Wait Time with Deviation
	float WaitTime = CurrentPatrolPoint->WaitTime;
	float WaitDeviation = CurrentPatrolPoint->WaitDeviation;

	float MinWaitTime = WaitTime - WaitDeviation;
	float MaxWaitTime = WaitTime + WaitDeviation;
	float RandomWaitTime = FMath::RandRange(MinWaitTime, MaxWaitTime);

	// Set Blackboard Value as Float
	BlackboardComp->SetValueAsFloat(BB_WaitTime.SelectedKeyName, RandomWaitTime);

	// Finish Execute
	return EBTNodeResult::Succeeded;
}
