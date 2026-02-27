// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/BTTask_EnterCautionState.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_EnterCautionState::UBTTask_EnterCautionState()
{
	NodeName = "Enter Caution State";
}

EBTNodeResult::Type UBTTask_EnterCautionState::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// Get Blackboard Component
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return EBTNodeResult::Failed;
	}

	// Set Blackboard Value as Bool (true)
	BlackboardComp->SetValueAsBool(BB_CautionKey.SelectedKeyName, true);

	// Finish Execute - Success
	return EBTNodeResult::Succeeded;
}
