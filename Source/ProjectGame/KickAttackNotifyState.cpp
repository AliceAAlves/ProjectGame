// Fill out your copyright notice in the Description page of Project Settings.


#include "KickAttackNotifyState.h"

#include "FightingCharacter.h"

#include "Engine.h"

void UKickAttackNotifyState::NotifyBegin(USkeletalMeshComponent * MeshComp, UAnimSequenceBase * Animation, float TotalDuration) {
	//GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Magenta, __FUNCTION__);

	if (MeshComp != NULL && MeshComp->GetOwner() != NULL) {
		AFightingCharacter* Player = Cast<AFightingCharacter>(MeshComp->GetOwner());
		if (Player != NULL) {
			Player->KickAttackStart();
		}
	}
}


void UKickAttackNotifyState::NotifyEnd(USkeletalMeshComponent * MeshComp, UAnimSequenceBase * Animation) {
	//GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Magenta, __FUNCTION__);

	if (MeshComp != NULL && MeshComp->GetOwner() != NULL) {
		AFightingCharacter* Player = Cast<AFightingCharacter>(MeshComp->GetOwner());
		if (Player != NULL) {
			Player->KickAttackEnd();
		}
	}
}
