// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_FindPathPoint.generated.h"

/**
 * 
 */
UCLASS()
class LIKETHIEF_API UBTTask_FindPathPoint : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBTTask_FindPathPoint();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blackboard")
	FBlackboardKeySelector BB_TargetVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blackboard")
	FBlackboardKeySelector BB_WaitTime;
};
