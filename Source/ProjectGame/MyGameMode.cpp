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
	

	

	//FTimerHandle UnusedHandle;
	//GetWorldTimerManager().SetTimer(UnusedHandle, this, &AMyGameMode::SpawnPlayerRecharge, FMath::RandRange(2, 5), true);
}

void AMyGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AMyGameMode::SpawnEnemy()
{
	/*float RandX = FMath::RandRange(Spawn_X_Min, Spawn_X_Max);
	float RandY = FMath::RandRange(Spawn_Y_Min, Spawn_Y_Max);

	FVector SpawnPosition = FVector(RandX, RandY, Spawn_Z);
	FRotator SpawnRotation = FRotator(0.0f, 0.0f, 0.0f);

	GetWorld()->SpawnActor(PlaywerRecharge, &SpawnPosition, &SpawnRotation);*/
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