// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_RotateGuardOnPoint.generated.h"

/**
 * 
 */
UCLASS()
class LIKETHIEF_API UBTTask_RotateGuardOnPoint : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBTTask_RotateGuardOnPoint();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
