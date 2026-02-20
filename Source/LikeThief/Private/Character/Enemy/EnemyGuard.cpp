// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/EnemyGuard.h"

AEnemyGuard::AEnemyGuard()
{
	PatrolPathBehavior = EPatrolPathEnding::Loop;
	CurrentPathIndex = 0;
}
