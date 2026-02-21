// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/BTTask_ReachedPathPoint.h"
#include "Character/Enemy/EnemyGuard.h"
#include "AIController.h"

UBTTask_ReachedPathPoint::UBTTask_ReachedPathPoint()
{
	NodeName = "Reached Path Point";
}

EBTNodeResult::Type UBTTask_ReachedPathPoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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
	AEnemyGuard* GuardEnemyPawn = Cast<AEnemyGuard>(ControlledPawn);
	if (!GuardEnemyPawn)
	{
		return EBTNodeResult::Failed;
	}

	// Calculate next index
	int32 NextIndex = GuardEnemyPawn->CurrentPathIndex + GuardEnemyPawn->PathDirection;

	// Branch: Is Valid Index?
	if (GuardEnemyPawn->PatrolPath.IsValidIndex(NextIndex))
	{
		// True: Set CurrentPathIndex
		GuardEnemyPawn->CurrentPathIndex = NextIndex;

		// Finish Execute - Success
		return EBTNodeResult::Succeeded;
	}
	else
	{
		// False: Switch on EPatrolPathEnding
		switch (GuardEnemyPawn->PatrolPathBehavior)
		{
		case EPatrolPathEnding::DoNothing:
		{
			// Do Nothing - Finish Execute Success
			return EBTNodeResult::Succeeded;
		}

		case EPatrolPathEnding::PatrolBack:
		{
			// Set Path Direction (multiply by -1)
			GuardEnemyPawn->PathDirection = GuardEnemyPawn->PathDirection * -1;

			// Set CurrentPathIndex to next valid index
			GuardEnemyPawn->CurrentPathIndex = GuardEnemyPawn->CurrentPathIndex + GuardEnemyPawn->PathDirection;

			// Finish Execute - Success
			return EBTNodeResult::Succeeded;
		}

		case EPatrolPathEnding::Loop:
		{
			// Set Current Path Index to 0
			GuardEnemyPawn->CurrentPathIndex = 0;

			// Finish Execute - Success
			return EBTNodeResult::Succeeded;
		}

		default:
		{
			return EBTNodeResult::Failed;
		}
		}
	}
}
