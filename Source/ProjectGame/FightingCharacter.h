// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Blueprint/UserWidget.h"
#include "Components/BoxComponent.h"

#include <unordered_map>
#include <vector>
#include <map>

#include "FightingCharacter.generated.h"

UENUM(BlueprintType)
enum ReactType
{
	NoReact,
	FaceHit,
	StomachHit
};

UCLASS()
class PROJECTGAME_API AFightingCharacter : public ACharacter
{
	GENERATED_BODY()


public:
	// Sets default values for this character's properties
	AFightingCharacter();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
		USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
		UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
		UCameraComponent* Camera2;


	/**** Flags ****/
	UPROPERTY(BlueprintReadOnly, Category = Attack)
		bool IsAttacking;
	UPROPERTY(BlueprintReadOnly, Category = Attack)
		bool IsBlocking;
	UPROPERTY(BlueprintReadOnly, Category = Attack)
		bool IsDucking;
	UPROPERTY(BlueprintReadOnly, Category = Attack)
		bool MoveModPressed;
	UPROPERTY(BlueprintReadOnly, Category = Attack)
		bool TauntPressed;
	UPROPERTY(BlueprintReadWrite, Category = Attack)
		bool CanAddNextComboAttack = true;
	UPROPERTY(BlueprintReadWrite, Category = Attack)
		bool CanMove = true;
	UPROPERTY(BlueprintReadWrite, Category = Attack)
		bool CanJump_ = true;
	UPROPERTY(BlueprintReadWrite, Category = Attack)
		bool CanAttack = true;
	UPROPERTY(BlueprintReadWrite, Category = Attack)
		bool CanBlock = true;
	UPROPERTY(BlueprintReadWrite, Category = Attack)
		bool CanDuck = true;
	UPROPERTY(BlueprintReadWrite, Category = Reaction)
		TEnumAsByte <ReactType> Reaction = ReactType::NoReact;

	UPROPERTY(BlueprintReadWrite, Category = Attack)
		FString ComboSequenceStr = TEXT("");

	void MoveForward(float Axis);
	void MoveRight(float Axis);
	void Turn(float Val);
	void LookUp(float Val);
	void Run();
	void StopRunning();
	void JumpChecking();
	void Attack1();
	void StopAttack1();
	void Attack2();
	void StopAttack2();
	void Block();
	void StopBlocking();
	void Duck();
	void StopDucking();
	void MoveMod();
	void StopMoveMod();
	void Taunt();
	void StopTaunt();
	void ChangeCamera();
	void SetTargetEnemy(AFightingCharacter* enemy);
	void RotateToTarget(float DeltaTime);
	AFightingCharacter* GetTargetEnemy();
	bool bDefeated;

	UFUNCTION(BlueprintCallable, Category = Animation)
		float GetSpeedForAnimation(float delta_time);

	UFUNCTION(BlueprintCallable, Category = Animation)
		FVector GetTargetSocketLocation(FString MontageName);

	UFUNCTION(BlueprintCallable, Category = Attack)
		void ClearComboSequence();


	/******************* Collision Boxes ************************/

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Collision, meta = (AllowPrivateAccess = "true"))
		USceneComponent* CollisionBoxes;

	/** Weapon Collision Boxes**/

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Collision, meta = (AllowPrivateAccess = "true"))
		UBoxComponent* RightFistCollisionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Collision, meta = (AllowPrivateAccess = "true"))
		UBoxComponent* LeftFistCollisionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Collision, meta = (AllowPrivateAccess = "true"))
		UBoxComponent* RightFootCollisionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Collision, meta = (AllowPrivateAccess = "true"))
		UBoxComponent* LeftFootCollisionBox;

	/** Damage Collision Boxes**/

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Collision, meta = (AllowPrivateAccess = "true"))
		UBoxComponent* HeadCollisionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Collision, meta = (AllowPrivateAccess = "true"))
		UBoxComponent* ChestCollisionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Collision, meta = (AllowPrivateAccess = "true"))
		UBoxComponent* TorsoCollisionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Collision, meta = (AllowPrivateAccess = "true"))
		UBoxComponent* HipsCollisionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Collision, meta = (AllowPrivateAccess = "true"))
		UBoxComponent* RightArmCollisionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Collision, meta = (AllowPrivateAccess = "true"))
		UBoxComponent* LeftArmCollisionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Collision, meta = (AllowPrivateAccess = "true"))
		UBoxComponent* RightForearmCollisionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Collision, meta = (AllowPrivateAccess = "true"))
		UBoxComponent* LeftForearmCollisionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Collision, meta = (AllowPrivateAccess = "true"))
		UBoxComponent* RightThighCollisionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Collision, meta = (AllowPrivateAccess = "true"))
		UBoxComponent* LeftThighCollisionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Collision, meta = (AllowPrivateAccess = "true"))
		UBoxComponent* RightLegCollisionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Collision, meta = (AllowPrivateAccess = "true"))
		UBoxComponent* LeftLegCollisionBox;

	/* Attacks Functions */

	void PunchAttackStart();
	void PunchAttackEnd();
	void KickAttackStart();
	void KickAttackEnd();

	/* React Function */

	void ReactionStart(FString CollisionBoxName);



	/**
	* Triggered when the collision hits event fires between our weapon and enemy entities
	*/
	UFUNCTION()
		void OnAttackHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	/**
	* Triggered when the collider overlaps another component
	*/
	UFUNCTION()
		void OnAttackOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);

	/**
	* Triggered when the collider stops overlaping another component
	*/
	UFUNCTION()
		void OnAttackOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	void CollisionBoxesInit();
	void AttachCollisionBoxesToSockets();
	
	float speedForAnimation;
	std::vector<UBoxComponent*> DamageCollisionBoxes;
	std::vector<UBoxComponent*> WeaponCollisionBoxes;
	std::map <FString, FString> DamageCBCategory;
	std::map <FString, FString> WeaponCBCategory;

	AFightingCharacter* TargetEnemy;

	bool bAttack1;
	bool bAttack2;
	bool bIsRunning;

	float MaxWalkSpeed = 40.0f;
	float MaxRunSpeed = 200.0f;

	FVector Cam2Location;
	FVector Cam2LookAt;
	FRotator ControllerRotation;
	float Cam2Distance = 300;

	APlayerController* ThisPlayerController;
	bool IsPlayableChar = false;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};

float get_random_float();
