// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FightingCharacter.h"
#include "GameFramework/GameMode.h"
#include "MyGameMode.generated.h"

/**
 * Personalised game mode that spawns an enemy and sets the player character and the enemy as each other's target.
 */
UCLASS()
class PROJECTGAME_API AMyGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	/** Default constructor. */
	AMyGameMode();

	/** Called when the game starts */
	virtual void BeginPlay() override;

	/** Called every frame */
	virtual void Tick(float DeltaTime) override;

	/** Class of the enemy character as subclass of Pawn. Can be set in the Blueprint */
	UPROPERTY(EditAnywhere)
	TSubclassOf<APawn> EnemyClass;

	/** Position to spawn the enemy at. Can be set in the Blueprint */
	UPROPERTY(EditAnywhere)
	FVector EnemyStartPosition;

	/** Enemy Rotation when spawned. Can be set in the Blueprint */
	UPROPERTY(EditAnywhere)
	FRotator EnemyStartRotation;

	/** Pointer to the player's character */
	UPROPERTY(BlueprintReadOnly)
	AFightingCharacter* Player;

	/** Pointer to the enemy character spawned in BeginPlay() */
	UPROPERTY(BlueprintReadOnly)
	AFightingCharacter* Enemy;

	/** Class of the UI widget. Can be set in the Blueprint */
	UPROPERTY(EditAnywhere, Category = "UI HUD")
	TSubclassOf<UUserWidget> HealthBar_Widget_Class;

	/** Pointer to the UI widget object */
	UPROPERTY(BlueprintReadOnly, Category = "UI HUD")
	UUserWidget* HealthBar_Widget;

	/**
	 * Spawns an enemy character of EnemyClass at EnemyStartPosition. 
	 * If EnemyClass is AFightingCharacter, then sets the player character and the enemy as each other's target.
	 */
	void SpawnEnemy();
	
	
};
