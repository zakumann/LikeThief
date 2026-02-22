// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/BTTask_RotateGuardOnPoint.h"
#include "Character/Enemy/EnemyGuard.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"

UBTTask_RotateGuardOnPoint::UBTTask_RotateGuardOnPoint()
{
	NodeName = "Rotate Guard On Point";
}

EBTNodeResult::Type UBTTask_RotateGuardOnPoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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

	// Check if PatrolPath is valid
	if (!GuardEnemyPawn->PatrolPath.IsValidIndex(GuardEnemyPawn->CurrentPathIndex))
	{
		return EBTNodeResult::Failed;
	}

	// Get current PathPoint
	APathPoint* CurrentPathPoint = GuardEnemyPawn->PatrolPath[GuardEnemyPawn->CurrentPathIndex];
	if (!CurrentPathPoint)
	{
		return EBTNodeResult::Failed;
	}

	// Get Root Component
	USceneComponent* RootComponent = GuardEnemyPawn->GetRootComponent();
	if (!RootComponent)
	{
		return EBTNodeResult::Failed;
	}

	// Get World Location
	FVector TargetRelativeLocation = RootComponent->GetComponentLocation();

	// Get Actor Rotation from PathPoint
	FRotator TargetRelativeRotation = CurrentPathPoint->GetActorRotation();

	// Move Component To
	RootComponent->SetWorldLocationAndRotation(TargetRelativeLocation, TargetRelativeRotation);

	// Finish Execute - Success
	return EBTNodeResult::Succeeded;
}
