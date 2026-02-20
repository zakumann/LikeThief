// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/EnemyAI.h"

void AEnemyAI::BeginPlay()
{
	Super::BeginPlay();

	if (BehaviorTree)
	{
		RunBehaviorTree(BehaviorTree);
	}
}
