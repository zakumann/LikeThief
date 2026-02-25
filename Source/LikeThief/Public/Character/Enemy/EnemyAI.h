// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "EnemyAI.generated.h"

/**
 * 
 */
UCLASS()
class LIKETHIEF_API AEnemyAI : public AAIController
{
	GENERATED_BODY()

public:
	AEnemyAI();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	UBehaviorTree* BehaviorTree;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	UAIPerceptionComponent* AIPerception;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	UAISenseConfig_Sight* SightConfig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	UAISenseConfig_Hearing* HearingConfig;

private:
	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	void HandleSense(AActor* SensedActor, const FAIStimulus& Stimulus);
};
