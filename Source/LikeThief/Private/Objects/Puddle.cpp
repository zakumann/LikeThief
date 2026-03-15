// Fill out your copyright notice in the Description page of Project Settings.


#include "Objects/Puddle.h"
#include "Components/DecalComponent.h"
#include "Components/SphereComponent.h"

// Sets default values
APuddle::APuddle()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// Create the decal component and attach it to the scene root
	PuddleDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("Decal"));
	PuddleDecal->SetupAttachment(RootComponent);

	// Create the sphere component and attach it to the scene root
	OverlapSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	OverlapSphere->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void APuddle::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void APuddle::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

