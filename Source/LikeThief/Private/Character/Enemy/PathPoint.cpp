// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/PathPoint.h"

// Sets default values
APathPoint::APathPoint()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create Sphere Collision
	SphereCollision = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollision"));
	SphereCollision->SetSphereRadius(50.0f);
	RootComponent = SphereCollision;

	// Create Arrow Component
	Arrow = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	Arrow->SetupAttachment(RootComponent);
	Arrow->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
}

// Called when the game starts or when spawned
void APathPoint::BeginPlay()
{
	Super::BeginPlay();
	
}


