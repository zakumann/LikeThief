// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_ReachedPathPoint.generated.h"

/**
 * 
 */
UCLASS()
class LIKETHIEF_API UBTTask_ReachedPathPoint : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBTTask_ReachedPathPoint();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
