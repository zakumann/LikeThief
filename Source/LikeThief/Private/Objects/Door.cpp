// Fill out your copyright notice in the Description page of Project Settings.


#include "Objects/Door.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/KismetSystemLibrary.h"

// Sets default values
ADoor::ADoor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create Root Component
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// Create DoorFrame
	DoorFrame = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorFrame"));
	DoorFrame->SetupAttachment(RootComponent);

	DoorFrame->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);

	// Create Door
	Door = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Door"));
	Door->SetupAttachment(DoorFrame);
	Door->SetRelativeLocation(ClosedRelativeLocation);
	Door->SetRelativeRotation(ClosedRelativeRotation);

	Door->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
}

// Called when the game starts or when spawned
void ADoor::BeginPlay()
{
	Super::BeginPlay();
	
	// Setup Timeline
	if (DoorCurve)
	{
		FOnTimelineFloat TimelineProgress;
		TimelineProgress.BindDynamic(this, &ADoor::UpdateDoorMovement);
		DoorTimeline.AddInterpFloat(DoorCurve, TimelineProgress);

		FOnTimelineEvent TimelineFinished;
		TimelineFinished.BindDynamic(this, &ADoor::OnDoorTimelineFinished);
		DoorTimeline.SetTimelineFinishedFunc(TimelineFinished);

		// Set ease in/out
		DoorTimeline.SetTimelineLength(DoorMoveTime);
	}
}

// Called every frame
void ADoor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	DoorTimeline.TickTimeline(DeltaTime);
}

void ADoor::Interact_Implementation(AActor* Caller)
{
	UE_LOG(LogTemp, Warning, TEXT("Door Interacted by %s"), *Caller->GetName());

	// 문이 이미 움직이고 있다면 입력을 무시하고 함수 종료!
	if (DoorTimeline.IsPlaying())
	{
		return;
	}

	// Branch: bIsOpen?
	if (bIsOpen)
	{
		CloseDoor();
	}
	else
	{
		OpenDoor();
	}
}

void ADoor::OpenDoor()
{
	if (!Door)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Opening Door"));

	// Set Start and Target for interpolation
	StartLocation = Door->GetRelativeLocation();
	StartRotation = Door->GetRelativeRotation();
	TargetLocation = OpenRelativeLocation;
	TargetRotation = OpenRelativeRotation;

	// Play Timeline
	DoorTimeline.PlayFromStart();
}

void ADoor::CloseDoor()
{
	if (!Door)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Closing Door"));

	// Set Start and Target for interpolation
	StartLocation = Door->GetRelativeLocation();
	StartRotation = Door->GetRelativeRotation();
	TargetLocation = ClosedRelativeLocation;
	TargetRotation = ClosedRelativeRotation;

	// Play Timeline
	DoorTimeline.PlayFromStart();
}

void ADoor::UpdateDoorMovement(float Value)
{
	if (!Door)
	{
		return;
	}

	// Lerp Location
	FVector NewLocation = FMath::Lerp(StartLocation, TargetLocation, Value);
	Door->SetRelativeLocation(NewLocation);

	// Lerp Rotation
	FRotator NewRotation = FMath::Lerp(StartRotation, TargetRotation, Value);
	Door->SetRelativeRotation(NewRotation);
}

void ADoor::OnDoorTimelineFinished()
{
	// Toggle bIsOpen
	bIsOpen = !bIsOpen;

	UE_LOG(LogTemp, Warning, TEXT("Door Movement Finished. bIsOpen: %s"), bIsOpen ? TEXT("True") : TEXT("False"));
}