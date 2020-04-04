// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FightingCharacter.h"
#include "GameFramework/GameMode.h"
#include "MyGameMode.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTGAME_API AMyGameMode : public AGameMode
{
	GENERATED_BODY()

	AMyGameMode();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere)
		TSubclassOf<APawn> EnemyClass;

	UPROPERTY(EditAnywhere)
		FVector EnemyStartPosition;
	UPROPERTY(EditAnywhere)
		FRotator EnemyStartRotation;

	void SpawnEnemy();

private:
	AFightingCharacter* Player;
	AFightingCharacter* Enemy;
};
