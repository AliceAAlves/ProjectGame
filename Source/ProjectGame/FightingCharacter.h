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

/**
 * ReactType is an Enum that enumerates different types of reactions.
 * Direction from which the blow is received: F - Front / L - Left / R - Right
 * Force of the blow: S - Small / M - Medium / B - Big
 */
UENUM(BlueprintType)
enum ReactType
{
	NoReact,
	Face_FS, Face_FM, Face_FB, Face_LS, Face_LM, Face_LB, Face_RS, Face_RM, Face_RB,
	Torso_FS, Torso_FM, Torso_FB, Torso_LS, Torso_LM, Torso_LB, Torso_RS, Torso_RM, Torso_RB, Back
};


/**
 * FightingCharacters are Characters that are able to perform different fighting moves.
 * They have a set of collision boxes for different body parts and are able to react to collisions on different body parts.
 * They are also able to jump, duck, block and perform taunts.
 * They are designed to have health points that decrease as they receive hits from other FightingCharacters.
 * This class is heavily linked to the animation blueprint "FightingCharacterAnim_BP"
 *
 * @see ACharacter
 * @see FightingCharacterAnim_BP
 */
UCLASS()
class PROJECTGAME_API AFightingCharacter : public ACharacter
{
	GENERATED_BODY()


public:
	/** Default UObject constructor. */
	AFightingCharacter();

	/** Spring Arm that connects the follow camera to the Character's mesh so the camera follows the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	USpringArmComponent* CameraBoom;

	/** Camera that follows the character using the CameraBoom */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	UCameraComponent* FollowCamera;

	/** Default Camera that stays half way between the character and their target */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	UCameraComponent* Camera2;

	//~ Begin Pressed Keys Flags
	/** Tracks if one of the attack keys is being pressed while an attack action is possible */
	UPROPERTY(BlueprintReadOnly, Category = Attack)
	bool IsAttacking;

	/** Tracks if the block key is being pressed while blocking is possible */
	UPROPERTY(BlueprintReadOnly, Category = Attack)
	bool IsBlocking;

	/** Tracks if the duck key is being pressed while ducking is possible */
	UPROPERTY(BlueprintReadOnly, Category = Attack)
	bool IsDucking;

	/** 
	 * Tracks if the move modifier key is being pressed
	 * If true, a different attack will be performed when one of the attack keys is pressed
	 * It is only effective if it is the beginning of a combo sequence.
	 */
	UPROPERTY(BlueprintReadOnly, Category = Attack)
	bool MoveModPressed;

	/**
	 * Tracks if the taunt key is being pressed
	 * If true, a taunt action will be performed when one of the attack keys is pressed, instead of an attack
	 */
	UPROPERTY(BlueprintReadOnly, Category = Attack)
	bool TauntPressed;
	//~ End Pressed Keys Flags


	/**
	 * Tracks if the next move of a combo attack sequence can be added.
	 * It is set to false while an attack animation is playing, being set to true a period of time before the attack animation ends
	 * This period of time is determined by setting an AnimEnd notify on the Animation Montages a fraction of second before the end
	 * of animation, which will trigger the Animation Blueprint to set CanAddNextComboAttack back to true
	 */
	UPROPERTY(BlueprintReadWrite, Category = Attack)
	bool CanAddNextComboAttack = true;

	/** Tracks is character has been defeated or not */
	UPROPERTY(BlueprintReadOnly, Category = Getter)
		bool bDefeated;

	//~ Begin Flags for actions that can be performed at the current time
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
	//~End Flags for actions that can be performed at the current time

	/**
	 * Variable that tracks which reaction animation should be played.
	 * This variable will signal the animation Blueprint to play the specified reaction animation
	 */
	UPROPERTY(BlueprintReadWrite, Category = Reaction)
	TEnumAsByte <ReactType> Reaction = ReactType::NoReact;

	/**
	 * Variable that tracks the current Combo Sequence being performed.
	 * Example: "212" represents that Attack 2 was performed, followed by Attack 1, and is currently at Attack 2
	 * Each sequence will map to a different attack Animation Montage in the animation blueprint
	 * Attack 1 = "1", Attack 2 = "2", Move Modifier + Attack 1 = "3", Move Modifier + Attack 2 = "4"
	 * Taunt + Attack 1 = "5" or "55", Taunt + Attack 2 = "6" or "6"
	 */
	UPROPERTY(BlueprintReadWrite, Category = Attack)
	FString ComboSequenceStr = TEXT("");

	/**
	 * Moves the character in the forwards axis of the Controller. Controller rotation is the same as the active camera.
	 *
	 * @param Axis		Scale to apply to input. This can be used for analog input, ie a value of 0.5 applies half the normal value, while -1.0 would reverse the direction.
	 * @see APawn::AddMovementInput()
	 */
	void MoveForward(float Axis);

	/**
	 * Moves the character in the right axis of the Controller. Controller rotation is the same as the active camera.
	 *
	 * @param Axis		Scale to apply to input. This can be used for analog input, ie a value of 0.5 applies half the normal value, while -1.0 would reverse the direction.
	 * @see Pawn::AddMovementInput()
	 */
	void MoveRight(float Axis);

	/**
	 * Rotates the controller right or left, resulting in rotating the camera right or left.
	 * Only applicable to FollowCamera, as Camera2 stays fixed.
	 *
	 * @param Val		Scale to apply to input. This can be used for analog input, ie a value of 0.5 applies half the normal value, while -1.0 would reverse the direction.
	 */
	void Turn(float Val);

	/**
	 * Rotates the controller to look up or down, resulting in rotating the camera to look up or down.
	 * Only applicable to FollowCamera, as Camera2 stays fixed.
	 *
	 * @param Val		Scale to apply to input. This can be used for analog input, ie a value of 0.5 applies half the normal value, while -1.0 would reverse the direction.
	 */
	void LookUp(float Val);

	/**
	 * Makes character run instead of walk. Called when Run key is pressed.
	 * Sets IsRunning to true, which will increase MaxWalkSpeed of the CharacterMovementComponent to MaxRunSpeed. 
	 */
	void Run();

	/**
	 * Makes character stop running and go back to walking. Called when Run key is released.
	 * Sets IsRunning to false, which will gradually decrease MaxWalkSpeed of the CharacterMovementComponent back to the original value of MaxWalkSpeed.
	 */
	void StopRunning();

	/**
	 * Checks if the character can jump (CanJump_ is true) and if so calls ACharacter::Jump() and makes character Jump.
	 *
	 * @see ACharacter::Jump()
	 */
	void JumpChecking();

	/**
	 * Called when Attack 1 key is pressed.
	 * If CanAttack and CanAddNextComboAttack, adds the propriate string to the end of ComboSequenceStr
	 * and sets IsAttacking to true, which signals the animation blueprint to be play the corresponding Animation Montage
	 * animation blueprint to be play the corresponding Animation Montage
	 */
	UFUNCTION(BlueprintCallable, Category = Behaviour)
	void Attack1();

	/** Called when Attack 1 key is released. Sets IsAttacking to false */
	UFUNCTION(BlueprintCallable, Category = Behaviour)
	void StopAttack1();

	/**
	* Called when Attack 2 key is pressed.
	* If CanAttack and CanAddNextComboAttack, adds the propriate string to the end of ComboSequenceStr
	* and sets IsAttacking to true, which signals the animation blueprint to be play the corresponding Animation Montage
	*/
	UFUNCTION(BlueprintCallable, Category = Behaviour)
	void Attack2();

	/** Called when Attack 2 key is released. Sets IsAttacking to false */
	UFUNCTION(BlueprintCallable, Category = Behaviour)
	void StopAttack2();

	/**
	 * Called when Block key is pressed.
	 * If CanBlock sets IsBlocking to true, which signals the animation blueprint to be play the blocking Animation Montage
	 */
	UFUNCTION(BlueprintCallable, Category = Behaviour)
	void Block();

	/** Called when Block key is released. Sets IsBlocking to false */
	UFUNCTION(BlueprintCallable, Category = Behaviour)
	void StopBlocking();

	/**
	 * Called when Duck key is pressed.
	 * If CanDuck sets IsDucking to true, which signals the animation blueprint to be play the ducking Animation Montage
	 */
	UFUNCTION(BlueprintCallable, Category = Behaviour)
	void Duck();
	
	/** Called when Duck key is released. Sets IsDucking to false */
	UFUNCTION(BlueprintCallable, Category = Behaviour)
	void StopDucking();

	/** Called when MoveMod key is pressed. Sets MoveModPressed to true, so an alternative attack move can be performed */
	UFUNCTION(BlueprintCallable, Category = Behaviour)
	void MoveMod();

	/** Called when MoveMod key is released. Sets MoveModPressed back to false */
	UFUNCTION(BlueprintCallable, Category = Behaviour)
	void StopMoveMod();

	/** Called when Taunt key is pressed. Sets TauntPressed to true, so an taunt animation can be played instead of an attack */
	UFUNCTION(BlueprintCallable, Category = Behaviour)
	void Taunt();

	/** Called when Taunt key is released. Sets TauntPressed back to false */
	UFUNCTION(BlueprintCallable, Category = Behaviour)
	void StopTaunt();

	/** Changes the active camera between FollowCamera and Camera2 */
	void ChangeCamera();

	/**
	 * Sets another AFightingCharacter as the target enemy
	 *
	 * @param enemy		FightingCharacter to be set as the target enemy
	 */
	void SetTargetEnemy(AFightingCharacter* enemy);

	/**
	 * Gradually rotates the character to target enemy, if the character is moving (but is not running).
	 * Called every frame.
	 *
	 * @param DeltaTime		delta time so the gradual rotation can be calculated
	 */
	void RotateToTarget(float DeltaTime);

	/**
	 * Returns the Weapon velocity of the specified Weapon Collision Box Component (fists or feet collision boxes)
	 *
	 * @param WeaponComponent		Weapon Collision Box Component (fists or feet collision boxes)
	 * @return Velocity of the specified Weapon Component
	 */
	float GetWeaponVelocity(UPrimitiveComponent* WeaponComponent);


	/**
	 * Returns the character velocity to be used as the speed variable of the idle/walk Blend Space.
	 * If there is an abrupt change in velocity, the speed for animation is changed gradually
	 * to avoid abrupt changes in the animation
	 *
	 * @param DeltaTime
	 * @return character speed for animation Blend Space
	 */
	UFUNCTION(BlueprintCallable, Category = Animation)
	float GetSpeedForAnimation(float delta_time);

	/**
	 * Returns the world location of a socket of the target's skeleton mesh of the specified name.
	 * If socket name does not exist or is none return (0, 0, -100).
	 * If character is too far away or is not facing the enemy, also returns return (0, 0, -100).
	 * Used for the two-bone Inverse Kinetics in the animation blueprint, so an attack targets a specific part of the target's body
	 *
	 * @param SocketName	is the name of the socket of the target's skeleton mesh
	 * @return socket location (vector)
	 */
	UFUNCTION(BlueprintCallable, Category = Animation)
	FVector GetTargetSocketLocation(FName SocketName);

	/** Returns the current right foot location */
	UFUNCTION(BlueprintCallable, Category = Animation)
	FVector GetFootRLocation();

	/** Returns the current left foot location */
	UFUNCTION(BlueprintCallable, Category = Animation)
	FVector GetFootLLocation();

	/** Clears the ComboSequenceStr, resetting it back to "". Called when an attack animation finishes and no new attack key has been pressed*/
	UFUNCTION(BlueprintCallable, Category = Attack)
	void ClearComboSequence();

	/** Returns the target enemy's current location */
	UFUNCTION(BlueprintCallable, Category = Getter)
	FVector GetEnemyLocation();

	/** Returns TargetEnemy */
	UFUNCTION(BlueprintCallable, Category = Getter)
	AFightingCharacter* GetTargetEnemy();

	/** Returns HealthPoints */
	UFUNCTION(BlueprintCallable, Category = Getter)
	float GetHealthPoints();

	/** Returns DamagePotential of the specified body part */
	UFUNCTION(BlueprintCallable, Category = Getter)
	float GetDamagePotential(FString bodypart);

	/** Impact velocity of the last attack that performed. If the attack never collided, then the value is zero */
	UPROPERTY(BlueprintReadOnly, Category = Getter)
	float LastAttackImpactVel = 0.0f;

	/** Number of health points deducted from the opponent's health points during the last attack performed */
	UPROPERTY(BlueprintReadOnly, Category = Getter)
	int LastAttackPoints = 0;

	//~ Begin Collision Boxes

	/** Object to group all collision boxes under it. Simply used for organization motives in the Blueprint */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Collision, meta = (AllowPrivateAccess = "true"))
		USceneComponent* CollisionBoxes;

	//~ Begin Weapon Collision Boxes
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Collision, meta = (AllowPrivateAccess = "true"))
		UBoxComponent* RightFistCollisionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Collision, meta = (AllowPrivateAccess = "true"))
		UBoxComponent* LeftFistCollisionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Collision, meta = (AllowPrivateAccess = "true"))
		UBoxComponent* RightFootCollisionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Collision, meta = (AllowPrivateAccess = "true"))
		UBoxComponent* LeftFootCollisionBox;
	//~ End Weapon Collision Boxes

	//~ Begin Damage Collision Boxes
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
	//~ End Damage Collision Boxes

	//~Begin Attacks Functions
	/** @see PunchAttackNotifyState, KickAttackNotifyState */

	/**  Called when a PunchAttackNotifyState begins. Sets fists collision boxes collision profile to Weapon */
	void PunchAttackStart();

	/**  Called when a PunchAttackNotifyState ends. Sets fists collision boxes collision profile to No Collision */
	void PunchAttackEnd();

	/**  Called when a KickAttackNotifyState begins. Sets feet collision boxes collision profile to Weapon */
	void KickAttackStart();

	/**  Called when a KickAttackNotifyState ends. Sets feet collision boxes collision profile to No Collision */
	void KickAttackEnd();
	//~End Attacks Functions

	/** 
	 * Called when collision occurs between two characters, and this character is the one reaction rather than attacking.
	 * Sets the Reaction variable to the appropriate ReactType that trigger the animaiton blueprint to play the animation.
	 *
	 * @param attacker		pointer to the other actor that is attacking
	 * @param CollisionBox	pointer to the collision box of this character that suffered collision
	 * @param ImpactVel		impact velocity
	 * @param ImpactPoint	point of impact
	 * @param AttackName	Name of the attack Animation Montage being played by the attacker
	 */
	void ReactionStart(AActor* attacker, UPrimitiveComponent* CollisionBox, float ImpactVel, FVector ImpactPoint, FString AttackName);
	
	/** Triggered when a reaction animation ends. Sets Reaction back to NoReact and resets the actions that can be performed */
	UFUNCTION(BlueprintCallable, Category = React)
	void ReactionEnd();

	/**
	 * Deducts points from the health points of this character, based on the body part and impact velocity.
	 *
	 * @param CollisionBox	pointer to the collision box of this character that suffered collision
	 * @param ImpactVel		impact velocity
	 */
	void InflictDamage(UPrimitiveComponent* CollisionBox, float ImpactVel);

	/** Triggered when the collision hits event fires between Weapon collider and another component */
	UFUNCTION()
		void OnAttackHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	/** 
	 * Triggered when an Weapon collider overlaps another component.
	 * Triggers ReactionStart() and InflictDamage() on the other actor, if the other actor is a FightingCharacter.
	 */
	UFUNCTION()
		void OnAttackOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);

	/** Triggered when an Weapon collider stops overlaping another component */
	UFUNCTION()
		void OnAttackOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	/** Map that stores where each Damage Box is overlapping or not. Takes the box component's name as key */
	std::map <FString, bool> IsDamageBoxOverlapping;
	
	/** Time in seconds that one of the arms was last overlapped */
	float LastArmsOverlapTime;

	/** Flags that signal when a body part is hit. Used by HealthBar_UI blueprint to flash the respective body part when being hit */
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
	/** Called when the game starts or when spawned */
	virtual void BeginPlay() override;

	/** Initialises all collision boxes. Called in the constructor. */
	void CollisionBoxesInit();

	/** Attaches all collision boxes to the respective socket in the character's skeleton mesh. Called during BeginPlay() */
	void AttachCollisionBoxesToSockets();

	/** Initialises variables DamageCBCategories, DamagePotential, BaseDamage and LastDamageTakenTime. Called during BeginPlay() */
	void VariablesInit();
	
	/** Velocity used as the speed variable of the idle/walk Blend Space. @see GetSpeedForAnimation()*/
	float speedForAnimation;

	/** Vectors containing all the Damage Collision Boxes and all the Weapon Collision Boxes*/
	std::vector<UBoxComponent*> DamageCollisionBoxes;
	std::vector<UBoxComponent*> WeaponCollisionBoxes;

	/** Map that stores the category as a generalised body part of each Damage Box */
	std::map <FString, FString> DamageCBCategory;

	/** List of all the Damage Boxes categories of generalised body parts */
	std::vector <FString> DamageCBCategories;

	/** Pointer to the target enemy*/
	AFightingCharacter* TargetEnemy;

	/** Tracks if Attack 1 key is being pressed */
	bool bAttack1;

	/** Tracks if Attack 2 key is being pressed */
	bool bAttack2;

	/** Tracks if Run key is being pressed */
	bool bIsRunning;

	/** Maximum Walking and running speeds */
	float MaxWalkSpeed = 40.0f;
	float MaxRunSpeed = 320.0f;

	/** Word location of each foot (R - right, L - left) */
	FVector Foot_R_Location;
	FVector Foot_L_Location;

	/** If set to true the velocity of each fist is tracked */
	bool bTrackFistsVelocity;

	/** If set to true the velocity of each foot is tracked */
	bool bTrackFeetVelocity;

	/** Velocity of the each fist collision box. Are only updated if bTrackFistsVelocity is true */
	float RightFistVelocity;
	float LeftFistVelocity;

	/** Velocity of the each foot collision box. Are only updated if bTrackFeetVelocity is true */
	float RightFootVelocity;
	float LeftFootVelocity;

	/** Last position of each fist/foot collision box. Are only updated if bTrackFistsVelocity/bTrackFeetVelocity is true */
	FVector RightFistLastPos;
	FVector LeftFistLastPos;
	FVector RightFootLastPos;
	FVector LeftFootLastPos;

	/** Maximum velocity reached of each fist/foot collision box during the last (or current) attack performed */
	float RightFistVelocity_max;
	float LeftFistVelocity_max;
	float RightFootVelocity_max;
	float LeftFootVelocity_max;

	/** Location of Camera 2 */
	FVector Cam2Location;

	/** Look at location of Camera 2 */
	FVector Cam2LookAt;

	/** Controller rotation (equivalent to camera rotation */
	FRotator ControllerRotation;

	/** Default distance between Camera 2 and the segment connecting the character and their target */
	float Cam2Distance = 300;

	/** Stores a pointer to the this player's controller */
	APlayerController* ThisPlayerController;

	/** True if this is the playable character */
	bool IsPlayableChar = false;

	/**
	 * Maps each body part category to a DamagePotential.
	 * When inflicting damage the base damage is multiplied by this Damage Potential of the corresponding body part.
	 * The more a body part is hit, the Damage Potential is increased. Starts at 1.0 and caps at 3.0
	 */
	std::map <FString, float> DamagePotential;

	/**  Maps each body part category to a BaseDamage. Head has the biggest base damage, followed by torso, and the legs/arms have the lowest base damage */
	std::map <FString, float> BaseDamage;
	std::map <FString, float> LastDamageTakenTime;

	/** Tracks the current health points of the character. When 0 is reached, character is set as defeated */
	float HealthPoints = 1;

	/** The base increment for DamagePotential everytime a body part is hit */
	float PotentialIncrement = 0.05;

public:	
	/** Called every frame */
	virtual void Tick(float DeltaTime) override;

	/** Called to bind functionality to input */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};

/** Generates a random float between 0.0 and 1.0*/
float get_random_float();
