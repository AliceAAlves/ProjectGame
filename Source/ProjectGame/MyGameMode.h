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

public:

	AMyGameMode();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere)
		TSubclassOf<APawn> EnemyClass;

	UPROPERTY(EditAnywhere)
		FVector EnemyStartPosition;
	UPROPERTY(EditAnywhere)
		FRotator EnemyStartRotation;

	UPROPERTY(BlueprintReadOnly)
		AFightingCharacter* Player;

	UPROPERTY(BlueprintReadOnly)
		AFightingCharacter* Enemy;

	UPROPERTY(EditAnywhere, Category = "UI HUD")
		TSubclassOf<UUserWidget> HealthBar_Widget_Class;
	UUserWidget* HealthBar_Widget;

	void SpawnEnemy();

//private:
	
	
};
