// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_ExitCautionState.generated.h"

/**
 * 
 */
UCLASS()
class LIKETHIEF_API UBTTask_ExitCautionState : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_ExitCautionState();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blackboard")
	FBlackboardKeySelector BB_CautionState;
};
