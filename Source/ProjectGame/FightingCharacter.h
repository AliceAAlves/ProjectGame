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
	Face_FS, Face_FM, Face_FB, Face_LS, Face_LM, Face_LB, Face_RS, Face_RM, Face_RB,
	Torso_FS, Torso_FM, Torso_FB, Torso_LS, Torso_LM, Torso_LB, Torso_RS, Torso_RM, Torso_RB, Back
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
	UFUNCTION(BlueprintCallable, Category = Behaviour)
	void Attack1();
	UFUNCTION(BlueprintCallable, Category = Behaviour)
	void StopAttack1();
	UFUNCTION(BlueprintCallable, Category = Behaviour)
	void Attack2();
	UFUNCTION(BlueprintCallable, Category = Behaviour)
	void StopAttack2();
	UFUNCTION(BlueprintCallable, Category = Behaviour)
	void Block();
	UFUNCTION(BlueprintCallable, Category = Behaviour)
	void StopBlocking();
	UFUNCTION(BlueprintCallable, Category = Behaviour)
	void Duck();
	UFUNCTION(BlueprintCallable, Category = Behaviour)
	void StopDucking();
	UFUNCTION(BlueprintCallable, Category = Behaviour)
	void MoveMod();
	UFUNCTION(BlueprintCallable, Category = Behaviour)
	void StopMoveMod();
	UFUNCTION(BlueprintCallable, Category = Behaviour)
	void Taunt();
	UFUNCTION(BlueprintCallable, Category = Behaviour)
	void StopTaunt();
	void ChangeCamera();
	void SetTargetEnemy(AFightingCharacter* enemy);
	void RotateToTarget(float DeltaTime);
	float GetWeaponVelocity(UPrimitiveComponent* WeaponComponent);

	UPROPERTY(BlueprintReadOnly, Category = Getter)
		bool bDefeated;

	UFUNCTION(BlueprintCallable, Category = Animation)
		float GetSpeedForAnimation(float delta_time);

	UFUNCTION(BlueprintCallable, Category = Animation)
		FVector GetTargetSocketLocation(FName SocketName);

	UFUNCTION(BlueprintCallable, Category = Animation)
		FVector GetFootRLocation();

	UFUNCTION(BlueprintCallable, Category = Animation)
		FVector GetFootLLocation();

	UFUNCTION(BlueprintCallable, Category = Attack)
		void ClearComboSequence();

	UFUNCTION(BlueprintCallable, Category = Getter)
		FVector GetEnemyLocation();

	UFUNCTION(BlueprintCallable, Category = Getter)
		AFightingCharacter* GetTargetEnemy();

	UFUNCTION(BlueprintCallable, Category = Getter)
		float GetHealthPoints();

	UFUNCTION(BlueprintCallable, Category = Getter)
		float GetDamagePotential(FString bodypart);

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

	void ReactionStart(AActor* attacker, UPrimitiveComponent* CollisionBox, float ImpactVel, FVector ImpactPoint, FString AttackName);
	UFUNCTION(BlueprintCallable, Category = React)
		void ReactionEnd();

	void InflictDamage(UPrimitiveComponent* CollisionBox, float ImpactVel);

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

	/* For Impact Response */
	FVector ImpactDirection;
	float ImpactVelocity;
	float ImpactDeceleration = 10000.0f;
	std::map <FString, bool> IsDamageBoxOverlapping;
	float LastArmsOverlapTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Hit)
		bool HitHead = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Hit)
		bool HitTorso = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Hit)
		bool HitArmL = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Hit)
		bool HitArmR = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Hit)
		bool HitLegL = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Hit)
		bool HitLegR = false;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	void CollisionBoxesInit();
	void AttachCollisionBoxesToSockets();
	void VariablesInit();
	
	float speedForAnimation;
	std::vector<UBoxComponent*> DamageCollisionBoxes;
	std::vector<UBoxComponent*> WeaponCollisionBoxes;
	std::map <FString, FString> DamageCBCategory;
	std::map <FString, FString> WeaponCBCategory;
	std::vector <FString> DamageCBCategories;

	AFightingCharacter* TargetEnemy;

	bool bAttack1;
	bool bAttack2;
	bool bIsRunning;

	float MaxWalkSpeed = 40.0f;
	float MaxRunSpeed = 200.0f;

	FVector Foot_R_Location;
	FVector Foot_L_Location;

	bool bTrackFistsVelocity;
	bool bTrackFootsVelocity;
	float RightFistVelocity;
	float LeftFistVelocity;
	float RightFootVelocity;
	float LeftFootVelocity;
	FVector RightFistLastPos;
	FVector LeftFistLastPos;
	FVector RightFootLastPos;
	FVector LeftFootLastPos;

	float RightFistVelocity_max;
	float LeftFistVelocity_max;
	float RightFootVelocity_max;
	float LeftFootVelocity_max;

	FVector Cam2Location;
	FVector Cam2LookAt;
	FRotator ControllerRotation;
	float Cam2Distance = 300;

	APlayerController* ThisPlayerController;
	bool IsPlayableChar = false;

	std::map <FString, float> DamagePotential;
	std::map <FString, float> BaseDamage;
	std::map <FString, float> LastDamageTakenTime;
	float HealthPoints = 1;
	float PotentialIncrement = 0.05;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};

float get_random_float();
