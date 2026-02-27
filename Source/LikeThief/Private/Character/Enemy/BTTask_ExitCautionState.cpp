// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/BTTask_ExitCautionState.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_ExitCautionState::UBTTask_ExitCautionState()
{
	NodeName = "Exit Caution State";
}

EBTNodeResult::Type UBTTask_ExitCautionState::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// Get Blackboard Component
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return EBTNodeResult::Failed;
	}

	// Set Blackboard Value as Bool (false)
	BlackboardComp->SetValueAsBool(BB_CautionState.SelectedKeyName, false);

	// Finish Execute - Success
	return EBTNodeResult::Succeeded;
}
