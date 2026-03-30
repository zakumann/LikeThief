// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/TimelineComponent.h"
#include "Character/InteractionInterface.h"
#include "Door.generated.h"

UCLASS()
class LIKETHIEF_API ADoor : public AActor, public IInteractionInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADoor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// === IInteractionInterface Implementation ===
	virtual void Interact_Implementation(AActor* Caller) override;

private:
	// === Components ===
	UPROPERTY(VisibleAnywhere, Category = "Components")
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent* DoorFrame;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent* Door;

	// === Door State ===
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Door", meta = (AllowPrivateAccess = "true"))
	bool bIsOpen = false;

	// === Door Movement ===
	UPROPERTY(EditAnywhere, Category = "Door")
	FVector ClosedRelativeLocation = FVector(0.0f, 50.0f, 0.0f);

	UPROPERTY(EditAnywhere, Category = "Door")
	FRotator ClosedRelativeRotation = FRotator(0.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, Category = "Door")
	FVector OpenRelativeLocation = FVector(0.0f, 50.0f, 0.0f);

	UPROPERTY(EditAnywhere, Category = "Door")
	FRotator OpenRelativeRotation = FRotator(0.0f, 90.0f, 0.0f);

	UPROPERTY(EditAnywhere, Category = "Door")
	float DoorMoveTime = 1.0f;

	// === Timeline ===
	FTimeline DoorTimeline;

	UPROPERTY(EditAnywhere, Category = "Door")
	UCurveFloat* DoorCurve;

	UFUNCTION()
	void UpdateDoorMovement(float Value);

	UFUNCTION()
	void OnDoorTimelineFinished();

	void OpenDoor();
	void CloseDoor();

	// === Movement Interpolation ===
	FVector StartLocation;
	FRotator StartRotation;
	FVector TargetLocation;
	FRotator TargetRotation;
};
