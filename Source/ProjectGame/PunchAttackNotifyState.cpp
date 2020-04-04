// Fill out your copyright notice in the Description page of Project Settings.


#include "PunchAttackNotifyState.h"

#include "FightingCharacter.h"

#include "Engine.h"

void UPunchAttackNotifyState::NotifyBegin(USkeletalMeshComponent * MeshComp, UAnimSequenceBase * Animation, float TotalDuration) {
	//GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Magenta, __FUNCTION__);

	if (MeshComp != NULL && MeshComp->GetOwner() != NULL) {
		AFightingCharacter* Player = Cast<AFightingCharacter>(MeshComp->GetOwner());
		if (Player != NULL) {
			Player->PunchAttackStart();
		}
	}
}


void UPunchAttackNotifyState::NotifyEnd(USkeletalMeshComponent * MeshComp, UAnimSequenceBase * Animation) {
	//GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Magenta, __FUNCTION__);

	if (MeshComp != NULL && MeshComp->GetOwner() != NULL) {
		AFightingCharacter* Player = Cast<AFightingCharacter>(MeshComp->GetOwner());
		if (Player != NULL) {
			Player->PunchAttackEnd();
		}
	}
}

