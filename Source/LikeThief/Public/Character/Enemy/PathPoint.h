// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "Components/ArrowComponent.h"
#include "PathPoint.generated.h"

UCLASS()
class LIKETHIEF_API APathPoint : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APathPoint();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* SphereCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UArrowComponent* Arrow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	float WaitTime = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	float WaitDeviation = 1.0f;

};
