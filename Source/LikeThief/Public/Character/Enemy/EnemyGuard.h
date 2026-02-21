// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/Enemy/EnemyBase.h"
#include "Character/Enemy/PathPoint.h"
#include "EnemyGuard.generated.h"

UENUM(BlueprintType)
enum class EPatrolPathEnding : uint8
{
	DoNothing UMETA(DisplayName = "Do Nothing"),
	PatrolBack UMETA(DisplayName = "Patrol Back"),
	Loop UMETA(DisplayName = "Loop")
};

UCLASS()
class LIKETHIEF_API AEnemyGuard : public AEnemyBase
{
	GENERATED_BODY()

public:
	AEnemyGuard();

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Patrol")
	TArray<APathPoint*> PatrolPath;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Patrol")
	EPatrolPathEnding PatrolPathBehavior;
	
	UPROPERTY(BlueprintReadWrite, Category = "Patrol")
	int32 CurrentPathIndex = 0;

	//PathDirection
	UPROPERTY(BlueprintReadWrite, Category = "Patrol")
	int32 PathDirection = 1;
};
