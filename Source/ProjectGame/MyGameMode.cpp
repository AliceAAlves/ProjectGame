// Fill out your copyright notice in the Description page of Project Settings.


#include "MyGameMode.h"
#include "GameFramework/Actor.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"

#include "Engine.h"

AMyGameMode::AMyGameMode() {
	
	PrimaryActorTick.bCanEverTick = true;
}

void AMyGameMode::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();

	if (World != NULL) {
		Player = Cast<AFightingCharacter>(UGameplayStatics::GetPlayerPawn(World, 0));
		SpawnEnemy();
	}
	
	if (HealthBar_Widget_Class != nullptr) {
		HealthBar_Widget = CreateWidget(World, HealthBar_Widget_Class);
		HealthBar_Widget->AddToViewport();
	}
}

void AMyGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AMyGameMode::SpawnEnemy()
{
	UWorld* World = GetWorld();

	if (EnemyClass != NULL && World != NULL) {
		AActor* EnemyActor = World->SpawnActor(EnemyClass, &EnemyStartPosition, &EnemyStartRotation);
		Enemy = Cast<AFightingCharacter>(EnemyActor);
		if (Enemy != NULL && Player != NULL) {
			Enemy->SetTargetEnemy(Player);
			Player->SetTargetEnemy(Enemy);
		}
	}
}