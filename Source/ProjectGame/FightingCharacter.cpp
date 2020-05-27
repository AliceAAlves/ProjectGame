// Fill out your copyright notice in the Description page of Project Settings.


#include "FightingCharacter.h"
#include "Engine/EngineTypes.h"
#include "Kismet/KismetMathLibrary.h"
#include "Math/UnrealMathUtility.h"
#include "RenderCore.h"

#include <vector>
#include <random>

#include "Engine.h"


AFightingCharacter::AFightingCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 600.0f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Initialising Cameras
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	Cam2LookAt = GetActorLocation() + FVector(100.0f, 0.0f, 0.0f);
	Cam2Location = Cam2LookAt + GetActorForwardVector().RotateAngleAxis(90, FVector(0.0f, 0.0f, 1.0f)).GetSafeNormal()*Cam2Distance;

	Camera2 = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera2"));
	Camera2->SetWorldLocation(Cam2Location);
	Camera2->SetupAttachment(RootComponent);
	Camera2->bUsePawnControlRotation = true;

	bDefeated = false;
	IsBlocking = false;

	// Initialise collision boxes
	CollisionBoxesInit();
}

void AFightingCharacter::BeginPlay()
{
	Super::BeginPlay(); 

	// Activate Camera 2 (default Camera)
	FollowCamera->Deactivate();
	Camera2->Activate();

	// Set the Camera Location as the Controller Rotation
	ControllerRotation = UKismetMathLibrary::FindLookAtRotation(Cam2Location, Cam2LookAt);
	ThisPlayerController = UGameplayStatics::GetPlayerController(this, 0);
	ThisPlayerController->SetControlRotation(ControllerRotation);

	IsPlayableChar = UGameplayStatics::GetPlayerPawn(GetWorld(), 0) == this;

	AttachCollisionBoxesToSockets();

	// Set Collision events for Weapon Collision Boxes
	for (UBoxComponent* weapon : WeaponCollisionBoxes) {
		weapon->OnComponentHit.AddDynamic(this, &AFightingCharacter::OnAttackHit);
		weapon->OnComponentBeginOverlap.AddDynamic(this, &AFightingCharacter::OnAttackOverlapBegin);
		weapon->OnComponentEndOverlap.AddDynamic(this, &AFightingCharacter::OnAttackOverlapEnd);
	}

	VariablesInit();
}

// Called every frame
void AFightingCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	RotateToTarget(DeltaTime);

	// Setting max speed on whether is running or not
	// If it's running character orients rotation to movement, but if it's only walking it does not
	if (bIsRunning) {
		GetCharacterMovement()->bOrientRotationToMovement = true;
		GetCharacterMovement()->MaxWalkSpeed = MaxRunSpeed;
	}
	else {
		GetCharacterMovement()->bOrientRotationToMovement = false;
		//If it's not running but current speed is more than the max walk speed, decrease it gradually
		if (GetVelocity().Size() > MaxWalkSpeed) {
			GetCharacterMovement()->MaxWalkSpeed -= GetCharacterMovement()->MaxAcceleration*DeltaTime;
		}
		else GetCharacterMovement()->MaxWalkSpeed = MaxWalkSpeed;
	}

	// Tracking velocity of fists/foots when punching/kicking
	if (bTrackFistsVelocity) {
		FVector currentPos = LeftFistCollisionBox->GetComponentLocation();
		LeftFistVelocity = (currentPos - LeftFistLastPos).Size()/DeltaTime;
		LeftFistLastPos = currentPos;

		currentPos = RightFistCollisionBox->GetComponentLocation();
		RightFistVelocity = (currentPos - RightFistLastPos).Size() / DeltaTime;
		RightFistLastPos = currentPos;

		//GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Blue, FString::Printf(TEXT("RightFistVelocity: %f"), RightFistVelocity));
		//GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Yellow, FString::Printf(TEXT("LeftFistLastPos: %f"), LeftFistVelocity));

		if (LeftFistVelocity > LeftFistVelocity_max) LeftFistVelocity_max = LeftFistVelocity;
		if (RightFistVelocity > RightFistVelocity_max) RightFistVelocity_max = RightFistVelocity;
	}

	if (bTrackFeetVelocity) {
		FVector currentPos = LeftFootCollisionBox->GetComponentLocation();
		LeftFootVelocity = (currentPos - LeftFootLastPos).Size() / DeltaTime;
		LeftFootLastPos = currentPos;

		currentPos = RightFootCollisionBox->GetComponentLocation();
		RightFootVelocity = (currentPos - RightFootLastPos).Size() / DeltaTime;
		RightFootLastPos = currentPos;

		//GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Blue, FString::Printf(TEXT("RightFootVelocity: %f"), RightFootVelocity));
		//GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Yellow, FString::Printf(TEXT("LeftFootVelocity: %f"), LeftFootVelocity));

		if (LeftFootVelocity > LeftFootVelocity_max) LeftFootVelocity_max = LeftFootVelocity;
		if (RightFootVelocity > RightFootVelocity_max) RightFootVelocity_max = RightFootVelocity;
	}

	// Track last time an arm was overlapping while blocking
	if (IsBlocking) {
		if(IsDamageBoxOverlapping[TEXT("RightArmCollisionBox")] || IsDamageBoxOverlapping[TEXT("RightForearmCollisionBox")]
			|| IsDamageBoxOverlapping[TEXT("LeftArmCollisionBox")] || IsDamageBoxOverlapping[TEXT("LeftForearmCollisionBox")])
			LastArmsOverlapTime = GetWorld()->GetTimeSeconds();
	}

	/*for (UBoxComponent* db : DamageCollisionBoxes) { // for debugging
		GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Yellow, FString::Printf(TEXT("%s is overlapping"), *db->GetName()));
	}*/


	//Set Camera 2 location and rotation
	if (IsPlayableChar && Camera2->IsActive() && TargetEnemy != NULL && ThisPlayerController != NULL) {
		FVector ToTargetDirection = TargetEnemy->GetActorLocation() - GetActorLocation();
		float distance = ToTargetDirection.Size();
		//GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Cyan, FString::Printf(TEXT("distance: %f"), distance));

		// If character are very distanced from each other add a distanceOffset,
		// so the camera is further away and both characters can been seen
		float distanceOffset = 0.0f;
		if (distance > 500.0f) {
			distanceOffset = (distance - 500.0f)*0.5;
		}

		// Look At is middle point between the character and the target enemy
		Cam2LookAt = GetActorLocation() + ToTargetDirection/2;

		// Location is to the right of Cam2LookAt (90º angle with ToTargetDirection) at a (Cam2Distance + distanceOffset) distance
		Cam2Location = Cam2LookAt + ToTargetDirection.RotateAngleAxis(90, FVector(0.0f, 0.0f, 1.0f)).GetSafeNormal() * (Cam2Distance + distanceOffset);
		Cam2LookAt.Z = 250;
		Cam2Location.Z = 250;		
		
		Camera2->SetWorldLocation(Cam2Location);
		ControllerRotation = UKismetMathLibrary::FindLookAtRotation(Cam2Location, Cam2LookAt);
		ThisPlayerController->SetControlRotation(ControllerRotation);
	}
	

	//UE_LOG(LogTemp, Warning, TEXT("speed: %f"), GetVelocity().Size());
}

// Called to bind functionality to input
void AFightingCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AFightingCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AFightingCharacter::MoveRight);

	PlayerInputComponent->BindAxis("Turn", this, &AFightingCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &AFightingCharacter::LookUp);

	PlayerInputComponent->BindAction("ChangeCamera", IE_Pressed, this, &AFightingCharacter::ChangeCamera);

	PlayerInputComponent->BindAction("Run", IE_Pressed, this, &AFightingCharacter::Run);
	PlayerInputComponent->BindAction("Run", IE_Released, this, &AFightingCharacter::StopRunning);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AFightingCharacter::JumpChecking);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("Attack1", IE_Pressed, this, &AFightingCharacter::Attack1);
	PlayerInputComponent->BindAction("Attack1", IE_Released, this, &AFightingCharacter::StopAttack1);

	PlayerInputComponent->BindAction("Attack2", IE_Pressed, this, &AFightingCharacter::Attack2);
	PlayerInputComponent->BindAction("Attack2", IE_Released, this, &AFightingCharacter::StopAttack2);

	PlayerInputComponent->BindAction("Block", IE_Pressed, this, &AFightingCharacter::Block);
	PlayerInputComponent->BindAction("Block", IE_Released, this, &AFightingCharacter::StopBlocking);

	PlayerInputComponent->BindAction("Duck", IE_Pressed, this, &AFightingCharacter::Duck);
	PlayerInputComponent->BindAction("Duck", IE_Released, this, &AFightingCharacter::StopDucking);

	PlayerInputComponent->BindAction("MoveMod", IE_Pressed, this, &AFightingCharacter::MoveMod);
	PlayerInputComponent->BindAction("MoveMod", IE_Released, this, &AFightingCharacter::StopMoveMod);

	PlayerInputComponent->BindAction("Taunt", IE_Pressed, this, &AFightingCharacter::Taunt);
	PlayerInputComponent->BindAction("Taunt", IE_Released, this, &AFightingCharacter::StopTaunt);

	
}

void AFightingCharacter::MoveForward(float Axis)
{
	if (!bDefeated && CanMove) {
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Axis);
	}
}

void AFightingCharacter::MoveRight(float Axis)
{
	if (!bDefeated  && CanMove){
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(Direction, Axis);
	}
}

void AFightingCharacter::Turn(float Val)
{
	if (FollowCamera->IsActive()) {
		AddControllerYawInput(Val);
	}
}

void AFightingCharacter::LookUp(float Val)
{
	if (FollowCamera->IsActive()) {
		AddControllerPitchInput(Val);
	}
}

void AFightingCharacter::Run()
{
	bIsRunning = true;
}

void AFightingCharacter::StopRunning()
{
	bIsRunning = false;
}

void AFightingCharacter::JumpChecking()
{
	if (!bDefeated && CanJump_) {
		Jump();
	}
}

void AFightingCharacter::Attack1()
{
	if (!bDefeated && CanAttack && !(GetCharacterMovement()->IsFalling())) {
		if (CanAddNextComboAttack) {
			if (IsDucking) ComboSequenceStr = TEXT("0");
			// Move modifier is only effecitve if it's the beginning of a new sequence
			else if (MoveModPressed && ComboSequenceStr.Equals(TEXT(""))) ComboSequenceStr = TEXT("3");
			else if (TauntPressed) {
				// Randomly chooses between two taunt animations
				if( get_random_float() <= 0.50) ComboSequenceStr = TEXT("5");
				else ComboSequenceStr = TEXT("55");
			}
			else ComboSequenceStr += TEXT("1");
			CanAddNextComboAttack = false;
			CanMove = false;
			CanBlock = false;
			CanJump_ = false;
			CanDuck = false;

			Foot_R_Location = GetMesh()->GetSocketLocation("foot_r");
			Foot_L_Location = GetMesh()->GetSocketLocation("foot_l");
		}
		IsAttacking = true;
	}
}

void AFightingCharacter::StopAttack1()
{
	bAttack1 = false;
	IsAttacking = false;
}

void AFightingCharacter::Attack2()
{
	if (!bDefeated && CanAttack && !(GetCharacterMovement()->IsFalling())) {
		if (CanAddNextComboAttack) { 
			if (IsDucking) ComboSequenceStr = TEXT("0");
			// Move modifier is only effecitve if it's the beginning of a new sequence
			else if (MoveModPressed && ComboSequenceStr.Equals(TEXT(""))) ComboSequenceStr = TEXT("4");
			else if (TauntPressed) {
				// Randomly chooses between two taunt animations
				if (get_random_float() <= 0.90) ComboSequenceStr = TEXT("6");
				else ComboSequenceStr = TEXT("66");
			}
			else ComboSequenceStr += TEXT("2");
			CanAddNextComboAttack = false;
			CanMove = false;
			CanBlock = false;
			CanJump_ = false;
			CanDuck = false;

			Foot_R_Location = GetMesh()->GetSocketLocation("foot_r");
			Foot_L_Location = GetMesh()->GetSocketLocation("foot_l");
		}
		IsAttacking = true;
	}
}

void AFightingCharacter::StopAttack2()
{
	bAttack2 = false;
	IsAttacking = false;
}

void AFightingCharacter::Block()
{
	if (!bDefeated && CanBlock && !(GetCharacterMovement()->IsFalling())) {
		IsBlocking = true;
		CanMove = false;
		CanAttack = false;
		CanJump_ = false;
	}
}

void AFightingCharacter::StopBlocking()
{
	if (IsBlocking) {
		IsBlocking = false;
		CanMove = true;
		CanAttack = true;
		CanJump_ = true;
	}
}

void AFightingCharacter::Duck()
{
	if (!bDefeated && CanDuck && !(GetCharacterMovement()->IsFalling())) {
		IsDucking = true;
		CanMove = false;
		CanJump_ = false;
		CanBlock = false;
	}
}

void AFightingCharacter::StopDucking()
{
	if (IsDucking) {
		IsDucking = false;
		CanMove = true;
		CanJump_ = true;
		CanBlock = true;
	}
}

void AFightingCharacter::MoveMod()
{
	MoveModPressed = true;
}


void AFightingCharacter::StopMoveMod()
{
	MoveModPressed = false;
}

void AFightingCharacter::Taunt()
{
	TauntPressed = true;
}

void AFightingCharacter::StopTaunt()
{
	TauntPressed = false;
}

void AFightingCharacter::ChangeCamera()
{
	if (FollowCamera->IsActive()) {
		Camera2->Activate();
		FollowCamera->Deactivate();
	}
	else {
		FollowCamera->Activate();
		Camera2->Deactivate();
	}
}

void AFightingCharacter::SetTargetEnemy(AFightingCharacter* enemy) {
	TargetEnemy = enemy;
}

AFightingCharacter* AFightingCharacter::GetTargetEnemy() {
	return TargetEnemy;
}

float AFightingCharacter::GetHealthPoints() {
	return HealthPoints;
}

float AFightingCharacter::GetDamagePotential(FString bodyPart) {
	return DamagePotential[bodyPart];
}

float AFightingCharacter::GetWeaponVelocity(UPrimitiveComponent* WeaponComponent) {

	float velocity = 0.0;

	if (WeaponComponent == LeftFistCollisionBox) velocity = LeftFistVelocity;
	else if(WeaponComponent == RightFistCollisionBox) velocity = RightFistVelocity;
	else if (WeaponComponent == LeftFootCollisionBox || WeaponComponent == LeftLegCollisionBox) velocity = LeftFootVelocity;
	else if (WeaponComponent == RightFootCollisionBox || WeaponComponent == RightLegCollisionBox) velocity = RightFootVelocity;

	return velocity;
}


void AFightingCharacter::PunchAttackStart()
{
	if (LeftFistCollisionBox == NULL || RightFistCollisionBox == NULL) return;

	LeftFistCollisionBox->SetCollisionProfileName("Weapon");
	LeftFistCollisionBox->SetNotifyRigidBodyCollision(true);
	LeftFistCollisionBox->SetGenerateOverlapEvents(true);

	RightFistCollisionBox->SetCollisionProfileName("Weapon");
	RightFistCollisionBox->SetNotifyRigidBodyCollision(true);
	RightFistCollisionBox->SetGenerateOverlapEvents(true);

	bTrackFistsVelocity = true;
	LeftFistLastPos = LeftFistCollisionBox->GetComponentLocation();
	RightFistLastPos = RightFistCollisionBox->GetComponentLocation();

	RightFistVelocity_max = LeftFistVelocity_max = 0;
	LastAttackImpactVel = LastAttackPoints = 0.0f;
}

void AFightingCharacter::PunchAttackEnd()
{
	LeftFistCollisionBox->SetCollisionProfileName("NoCollision");
	LeftFistCollisionBox->SetNotifyRigidBodyCollision(false);
	LeftFistCollisionBox->SetGenerateOverlapEvents(false);

	RightFistCollisionBox->SetCollisionProfileName("NoCollision");
	RightFistCollisionBox->SetNotifyRigidBodyCollision(false);
	RightFistCollisionBox->SetGenerateOverlapEvents(false);

	bTrackFistsVelocity = false;

	//GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Cyan, FString::Printf(TEXT("RightFistVelocity_max: %f"), RightFistVelocity_max));
	//GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Yellow, FString::Printf(TEXT("LeftFistVelocity_max: %f"), LeftFistVelocity_max));
}

void AFightingCharacter::KickAttackStart()
{
	if (LeftFootCollisionBox == NULL || RightFootCollisionBox == NULL || LeftLegCollisionBox == NULL || RightLegCollisionBox == NULL) return;

	LeftFootCollisionBox->SetCollisionProfileName("Weapon");
	LeftFootCollisionBox->SetNotifyRigidBodyCollision(true);
	LeftFootCollisionBox->SetGenerateOverlapEvents(true);

	RightFootCollisionBox->SetCollisionProfileName("Weapon");
	RightFootCollisionBox->SetNotifyRigidBodyCollision(true);
	RightFootCollisionBox->SetGenerateOverlapEvents(true);

	LeftLegCollisionBox->SetCollisionProfileName("Weapon");
	RightLegCollisionBox->SetCollisionProfileName("Weapon");

	bTrackFeetVelocity = true;
	LeftFootLastPos = LeftFootCollisionBox->GetComponentLocation();
	RightFootLastPos = RightFootCollisionBox->GetComponentLocation();

	RightFootVelocity_max = LeftFootVelocity_max = 0;
	LastAttackImpactVel = LastAttackPoints = 0.0f;
}

void AFightingCharacter::KickAttackEnd()
{
	LeftFootCollisionBox->SetCollisionProfileName("NoCollision");
	LeftFootCollisionBox->SetNotifyRigidBodyCollision(false);
	LeftFootCollisionBox->SetGenerateOverlapEvents(false);

	RightFootCollisionBox->SetCollisionProfileName("NoCollision");
	RightFootCollisionBox->SetNotifyRigidBodyCollision(false);
	RightFootCollisionBox->SetGenerateOverlapEvents(false);

	LeftLegCollisionBox->SetCollisionProfileName("DamageBox");
	RightLegCollisionBox->SetCollisionProfileName("DamageBox");

	bTrackFeetVelocity = false;

	//GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Green, FString::Printf(TEXT("RightFootVelocity_max: %f"), RightFootVelocity_max));
	//GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Orange, FString::Printf(TEXT("LeftFootVelocity_max: %f"), LeftFootVelocity_max));
}

void AFightingCharacter::ReactionStart(AActor* attacker, UPrimitiveComponent* CollisionBox, float ImpactVel, FVector ImpactPoint, FString AttackName) 
{
	Foot_R_Location = GetMesh()->GetSocketLocation("foot_r");
	Foot_L_Location = GetMesh()->GetSocketLocation("foot_l");

	FString CollisionBoxName = CollisionBox->GetName();
	FString hitArea = DamageCBCategory[CollisionBoxName];

	// Check if attacker is behind the character
	FVector actorToAttacker = attacker->GetActorLocation() - GetActorLocation();
	float cos = GetActorForwardVector().CosineAngle2D(actorToAttacker);
	bool isAttackerBehindActor = cos < -0.35;
	bool isAttackerInFrontOfActor = cos > 0.35;

	float current_time = GetWorld()->GetTimeSeconds();
	
	if (hitArea.Equals(TEXT("head"))) {
		//if (IsBlocking && isAttackerInFrontOfActor) return;
		
		// If the character is blocking and the arms have ovelapped in the last second, then don't react.
		if (IsBlocking && current_time - LastArmsOverlapTime < 1.0) return;
		else if (IsBlocking) StopBlocking();
		if (isAttackerBehindActor) Reaction = ReactType::Back; 
		else if (AttackName.Equals(TEXT("Attack_Duck_Punch")) || AttackName.Equals(TEXT("Attack_Punch_L_quick")) 
			|| AttackName.Equals(TEXT("Attack_Punch_Combo"))) {
			Reaction = ReactType::Face_FS;
		}
		else if ( AttackName.Equals(TEXT("Attack_Punch_R_quick"))) {
			Reaction = ReactType::Face_FM;
		}
		else if (AttackName.Equals(TEXT("Attack_Kick_scissors")) || AttackName.Equals(TEXT("Attack_Punch_L_uppercut")) || AttackName.Equals(TEXT("Attack_Punch_R_uppercut"))) {
			Reaction = ReactType::Face_FB;
		}
		else if (AttackName.Equals(TEXT("Attack_Kick_backwards_round"))) {
			Reaction = ReactType::Face_RB;
		}
		else if (AttackName.Equals(TEXT("Attack_Kick_R_high")) || AttackName.Equals(TEXT("Attack_Kick_R_roundhouse"))) {
			Reaction = ReactType::Face_LM;
		}
		else if (AttackName.Equals(TEXT("Attack_Kick_R_high_round")) || AttackName.Equals(TEXT("Attack_Punch_R_swing"))) {
			Reaction = ReactType::Face_LB;
		}
	}
	else if (hitArea.Equals(TEXT("torso"))) {
		if (IsBlocking) StopBlocking();
		if (isAttackerBehindActor) Reaction = ReactType::Back;
		else if (AttackName.Equals(TEXT("Attack_Kick_R_front")) || AttackName.Equals(TEXT("Attack_Kick_L_front"))) {
			Reaction = ReactType::Torso_FS;
		}
		else if (AttackName.Equals(TEXT("Attack_Kick_R_torso"))) {
			Reaction = ReactType::Torso_FM;
		}
		else if (AttackName.Equals(TEXT("Attack_Punch_L_uppercut")) || AttackName.Equals(TEXT("Attack_Punch_R_uppercut"))) {
			Reaction = ReactType::Torso_FB;
		}
		else if (AttackName.Equals(TEXT("Attack_Punch_R_hook"))) {
			Reaction = ReactType::Torso_LS;
		}
		else if (AttackName.Equals(TEXT("Attack_Kick_air")) //|| AttackName.Equals(TEXT("Attack_Kick_R_roundhouse")) || AttackName.Equals(TEXT("Attack_Kick_scissors"))
			|| AttackName.Equals(TEXT("Attack_Kick_L_roundhouse")) || AttackName.Equals(TEXT("Attack_Kick_R_high"))
			|| AttackName.Equals(TEXT("Attack_Punch_R_hook_momentum")) || AttackName.Equals(TEXT("Attack_Kick_R_mocap"))) {
			Reaction = ReactType::Torso_LM;
		}
		else if (AttackName.Equals(TEXT("Attack_Punch_L_hook"))) {
			Reaction = ReactType::Torso_RM;
		}

	}
	else if (hitArea.Equals(TEXT("chest"))) {
		//if (IsBlocking && isAttackerInFrontOfActor) return;

		// If the character is blocking and the arms have ovelapped in the last second, then don't react.
		if (IsBlocking && current_time - LastArmsOverlapTime < 1.0) return;
		else if (IsBlocking) StopBlocking();
		if (isAttackerBehindActor) Reaction = ReactType::Back;
		else if (AttackName.Equals(TEXT("Attack_Kick_air")) || AttackName.Equals(TEXT("Attack_Punch_Combo")) 
			|| AttackName.Equals(TEXT("Attack_Punch_R_hook")) || AttackName.Equals(TEXT("Attack_Punch_R_hook_momentum"))
			|| AttackName.Equals(TEXT("Attack_Kick_L_roundhouse")) || AttackName.Equals(TEXT("Attack_Kick_R_mocap"))) {
			Reaction = ReactType::Torso_LM;
		}
		else if (AttackName.Equals(TEXT("Attack_Punch_L_hook"))) {
			Reaction = ReactType::Torso_RM;
		}
	}

	// If a reaction was set then set other actions as not being able to be performed
	if (Reaction != ReactType::NoReact) {
		CanMove = false;
		CanJump_ = false;
		CanDuck = false;
		CanBlock = false;
		CanAttack = false;
	}
	
}

// Resetting actions that can be performed once a reaction animation ends
void AFightingCharacter::ReactionEnd() {
	Reaction = ReactType::NoReact;
	CanMove = true;
	CanJump_ = true;
	CanDuck = true;
	CanBlock = true;
	CanAttack = true;
	CanAddNextComboAttack = true;
	if (IsBlocking) StopBlocking();
}

void AFightingCharacter::InflictDamage(UPrimitiveComponent* CollisionBox, float ImpactVel)
{
	FString hit_area = DamageCBCategory[CollisionBox->GetName()];
	if (hit_area.IsEmpty()) return;

	// If the character is blocking and the the hit arae is head or chest
	// and the arms have ovelapped in the last second, then don't infliect damage.
	if (IsBlocking && (hit_area.Equals(TEXT("chest")) || hit_area.Equals(TEXT("head")))) {
		float current_time = GetWorld()->GetTimeSeconds();
		if (current_time - LastArmsOverlapTime < 1.0) return;
	}

	if (hit_area.Equals(TEXT("chest"))) hit_area = TEXT("torso");

	float current_time = GetWorld()->GetTimeSeconds();

	// Only inflict damage if it's been more than 0.5 seconds since the last time this hit area has damage received
	if (current_time - LastDamageTakenTime[hit_area] > 0.5) {
		//GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Magenta, FString::Printf(TEXT("Hit Area: %s (%s)"), *hit_area, *CollisionBox->GetName()));
		//GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Cyan, FString::Printf(TEXT("ImpactVel: %f "), ImpactVel)); 
		
		// Calculatinf damage taken based on ImpactVel and DamagePotential of the hit area
		float base_damage = BaseDamage[hit_area];
		float damage_multiplier = DamagePotential[hit_area];
		float damage_taken = base_damage * damage_multiplier * ImpactVel/800;
		HealthPoints -= damage_taken;
		if (HealthPoints < 0) { 
			HealthPoints = 0; 
			bDefeated = true;
		}

		// Increasing DamagePotential of the hit area, based on ImpactVel (min cap of PotentialIncrement)
		if(PotentialIncrement * ImpactVel / 600 > PotentialIncrement)
			DamagePotential[hit_area] += PotentialIncrement * ImpactVel / 600;
		else DamagePotential[hit_area] += PotentialIncrement;
		if (DamagePotential[hit_area] > 3) DamagePotential[hit_area] = 3;
		LastDamageTakenTime[hit_area] = current_time;

		//GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Yellow, FString::Printf(TEXT("damage taken: %d, DamagePotential: %f, HP: %d "), (int)(damage_taken * 1000), DamagePotential[hit_area], (int)(HealthPoints * 1000)));
	
		if (hit_area.Equals(TEXT("torso"))) HitTorso = true;
		else if (hit_area.Equals(TEXT("head"))) HitHead = true;
		else if (hit_area.Equals(TEXT("left_arm"))) HitArmL = true;
		else if (hit_area.Equals(TEXT("right_arm"))) HitArmR = true;
		else if (hit_area.Equals(TEXT("left_leg"))) HitLegL = true;
		else if (hit_area.Equals(TEXT("right_Leg"))) HitLegR = true;

		TargetEnemy->LastAttackPoints += (int)(damage_taken * 1000);
		if (ImpactVel > TargetEnemy->LastAttackImpactVel) TargetEnemy->LastAttackImpactVel = ImpactVel;
	}
}

void AFightingCharacter::OnAttackHit(UPrimitiveComponent * HitComponent, AActor * OtherActor, UPrimitiveComponent * OtherComp, FVector NormalImpulse, const FHitResult & Hit)
{
	//GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Magenta, __FUNCTION__);
}

void AFightingCharacter::OnAttackOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	if (OtherActor != this && OtherActor != NULL) {
		if (AFightingCharacter* enemy = Cast<AFightingCharacter>(OtherActor)) {
			FString hitArea = DamageCBCategory[OtherComp->GetName()];

			// SweepResult is unpopulated for OnOverlapBegin - a bug from Unreal. Solution to get the HitResult:
		    // https://answers.unrealengine.com/questions/165523/on-component-begin-overlap-sweep-result-not-popula.html

			TArray<FHitResult> AllResults;
			FHitResult Hit;

			// Get the location of this actor's overlapped component
			auto Start = OverlappedComponent->GetComponentLocation();
			// Get the location of the other component
			auto End = OtherComp->GetComponentLocation();
			// Use a slightly larger radius to ensure we find the same result
			auto CollisionRadius = FVector::Dist(Start, End) * 1.1f;

			// Now do a spherical sweep to find the overlap
			GetWorld()->SweepMultiByObjectType(AllResults, Start, End, FQuat::Identity, 0, FCollisionShape::MakeSphere(CollisionRadius),
				FCollisionQueryParams::FCollisionQueryParams(false) );

			// Finally check which hit result is the one from this event
			for (auto HitResult : AllResults)
			{
				if (OtherComp->GetUniqueID() == HitResult.GetComponent()->GetUniqueID()) {
					// A component with the same UniqueID means we found our overlap!
					Hit = HitResult;
					break;
				}
			}
			/************************************/

			//GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Blue, FString::Printf(TEXT("%s is overlapping"), *OtherComp->GetName()));
			enemy->IsDamageBoxOverlapping[OtherComp->GetName()] = true;

			// Inflict damage on enemy and start reaction for enemy
			enemy->InflictDamage(OtherComp, GetWeaponVelocity(OverlappedComponent));
			if(GetCurrentMontage() != NULL) enemy->ReactionStart(this, OtherComp, GetWeaponVelocity(OverlappedComponent), Hit.ImpactPoint, GetCurrentMontage()->GetName());
		}
	}
}


void AFightingCharacter::OnAttackOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor != this && OtherActor != NULL) {
		if (AFightingCharacter* enemy = Cast<AFightingCharacter>(OtherActor)) {
			enemy->IsDamageBoxOverlapping[OtherComp->GetName()] = false;
		}
	}
}

void AFightingCharacter::RotateToTarget(float DeltaTime) {

	float speed = GetVelocity().Size();
	if (TargetEnemy != NULL && speed != 0) {
		FVector TargetLocation = TargetEnemy->GetActorLocation();
		TargetLocation.Z = GetActorLocation().Z;
		FRotator LookAt = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), TargetLocation);
		
		// Interpolate to desired rotation, so the change in rotation is gradual and not sudden
		FRotator NextRotation = FMath::RInterpTo(GetActorRotation(), LookAt, DeltaTime, 2.0f);

		SetActorRotation(NextRotation);
	}
	
}

float AFightingCharacter::GetSpeedForAnimation(float delta_time)
{
	float max_accel = GetCharacterMovement()->MaxAcceleration + 20;
	float anim_accel = GetCharacterMovement()->MaxAcceleration;
	
	FVector velocity = FVector(GetVelocity().X, GetVelocity().Y, 0.0f);
	float actual_speed = velocity.Size();
	float accel = (actual_speed - speedForAnimation)/delta_time;

	// If the acceleration of the actual speed is greater than the max acceleration allowed,
	// then change speedForAnimation gradually
	if (abs(accel) > max_accel) {
		if (accel < 0) anim_accel = -anim_accel;
		speedForAnimation += anim_accel * delta_time;
	}
	else { speedForAnimation = actual_speed; }

	// velocity.Size() is always positive, so the cos wih the forward vector is used to check if the character
	// is moving backwards. If so animation must be set as negative
	// (because the idle/walk Blend Space takes negative values for walking backwards)
	float cosAngle = GetActorForwardVector().CosineAngle2D(GetVelocity());

	if (cosAngle < 0) return -speedForAnimation;

	return speedForAnimation;
}

FVector AFightingCharacter::GetTargetSocketLocation(FName SocketName)
{
	FVector TargetLocation(0.0, 0.0, -100.0); // means no target

	// get enemy socket location
	if (TargetEnemy != NULL && !SocketName.IsNone()) {
		
		FVector SocketLocation = TargetEnemy->GetMesh()->GetSocketLocation(SocketName);
		
		FVector VectorToTarget = SocketLocation - GetActorLocation();
		float distance = VectorToTarget.Size();
		
		// check actor is close to enemy
		if (distance < 200.0) {
			float cosAngle = GetActorForwardVector().CosineAngle2D(VectorToTarget);

			// check angle to see if actor is facing enemy
			//if so return socket location
			if (cosAngle > 0.75) {
				TargetLocation = SocketLocation;
			}
		}

		/*if (this == UGameplayStatics::GetPlayerPawn(GetWorld(), 0)) { // for debugging
			GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Magenta, FString::Printf(TEXT("SocketName: %s"), *SocketName.ToString()));
			GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Yellow, FString::Printf(TEXT("TargetLocation.x: %f, y: %f, z: %f"), TargetLocation.X, TargetLocation.Y, TargetLocation.Z));
		}*/
	}
	return TargetLocation;
}


FVector AFightingCharacter::GetFootRLocation() {
	return Foot_R_Location;
}

FVector AFightingCharacter::GetFootLLocation() {
	return Foot_L_Location;
}

void AFightingCharacter::ClearComboSequence()
{
	ComboSequenceStr = TEXT("");
	CanAddNextComboAttack = true;
	CanMove = true;
	CanBlock = true;
	CanJump_ = true;
	CanDuck = true;
	CanAttack = true;
	//GEngine->AddOnScreenDebugMessage(-1, 4.5f, FColor::Orange, TEXT("CLEARED"));
}


void AFightingCharacter::CollisionBoxesInit() 
{
	CollisionBoxes = CreateDefaultSubobject<USceneComponent>(TEXT("CollisionBoxes"));
	
		
	/** Weapon Collision Boxes**/

	RightFistCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("RightFistCollisionBox"));
	WeaponCollisionBoxes.push_back(RightFistCollisionBox);

	LeftFistCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("LeftFistCollisionBox"));
	WeaponCollisionBoxes.push_back(LeftFistCollisionBox);

	RightFootCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("RightFootCollisionBox"));
	WeaponCollisionBoxes.push_back(RightFootCollisionBox);

	LeftFootCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("LeftFootCollisionBox"));
	WeaponCollisionBoxes.push_back(LeftFootCollisionBox);

	/** Damage Collision Boxes**/

	HeadCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("HeadCollisionBox"));
	DamageCollisionBoxes.push_back(HeadCollisionBox);
	DamageCBCategory[HeadCollisionBox->GetName()] = "head";

	ChestCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ChestCollisionBox"));
	DamageCollisionBoxes.push_back(ChestCollisionBox);
	DamageCBCategory[ChestCollisionBox->GetName()] = "chest";

	TorsoCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TorsoCollisionBox"));
	DamageCollisionBoxes.push_back(TorsoCollisionBox);
	DamageCBCategory[TorsoCollisionBox->GetName()] = "torso";

	HipsCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("HipsCollisionBox"));
	DamageCollisionBoxes.push_back(HipsCollisionBox);
	DamageCBCategory[HipsCollisionBox->GetName()] = "torso";

	RightArmCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("RightArmCollisionBox"));
	DamageCollisionBoxes.push_back(RightArmCollisionBox);
	DamageCBCategory[RightArmCollisionBox->GetName()] = "right_arm";

	RightForearmCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("RightForearmCollisionBox"));
	DamageCollisionBoxes.push_back(RightForearmCollisionBox);
	DamageCBCategory[RightForearmCollisionBox->GetName()] = "right_arm";

	LeftArmCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("LeftArmCollisionBox"));
	DamageCollisionBoxes.push_back(LeftArmCollisionBox);
	DamageCBCategory[LeftArmCollisionBox->GetName()] = "left_arm";

	LeftForearmCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("LeftForearmCollisionBox"));
	DamageCollisionBoxes.push_back(LeftForearmCollisionBox);
	DamageCBCategory[LeftForearmCollisionBox->GetName()] = "left_arm";

	RightThighCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("RightThighCollisionBox"));
	DamageCollisionBoxes.push_back(RightThighCollisionBox);
	DamageCBCategory[RightThighCollisionBox->GetName()] = "right_leg";

	RightLegCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("RightLegCollisionBox"));
	DamageCollisionBoxes.push_back(RightLegCollisionBox);
	WeaponCollisionBoxes.push_back(RightLegCollisionBox);
	DamageCBCategory[RightLegCollisionBox->GetName()] = "right_leg";

	LeftThighCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("LeftThighCollisionBox"));
	DamageCollisionBoxes.push_back(LeftThighCollisionBox);
	DamageCBCategory[LeftThighCollisionBox->GetName()] = "left_leg";

	LeftLegCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("LeftLegCollisionBox"));
	DamageCollisionBoxes.push_back(LeftLegCollisionBox);
	WeaponCollisionBoxes.push_back(LeftLegCollisionBox);
	DamageCBCategory[LeftLegCollisionBox->GetName()] = "left_leg";
	

	for (UBoxComponent* element : WeaponCollisionBoxes)
	{
		element->SetupAttachment(CollisionBoxes);
		element->SetCollisionProfileName("NoCollision");
		element->SetNotifyRigidBodyCollision(false);
		element->SetHiddenInGame(false);
	}

	for (UBoxComponent* element : DamageCollisionBoxes)
	{
		element->SetupAttachment(CollisionBoxes);
		element->SetCollisionProfileName("DamageBox");
		element->SetNotifyRigidBodyCollision(true);
		element->SetHiddenInGame(false);
		IsDamageBoxOverlapping[element->GetName()] = false;
	}

}

void AFightingCharacter::AttachCollisionBoxesToSockets()
{
	// attach collision components to sockets based on transformations definitions
	const FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, false);

	LeftFistCollisionBox->AttachToComponent(GetMesh(), AttachmentRules, "fist_l_collision");
	RightFistCollisionBox->AttachToComponent(GetMesh(), AttachmentRules, "fist_r_collision");
	RightFootCollisionBox->AttachToComponent(GetMesh(), AttachmentRules, "foot_r_collision");
	LeftFootCollisionBox->AttachToComponent(GetMesh(), AttachmentRules, "foot_l_collision");
	HeadCollisionBox->AttachToComponent(GetMesh(), AttachmentRules, "head_collision");
	ChestCollisionBox->AttachToComponent(GetMesh(), AttachmentRules, "chest_collision");
	TorsoCollisionBox->AttachToComponent(GetMesh(), AttachmentRules, "torso_collision");
	HipsCollisionBox->AttachToComponent(GetMesh(), AttachmentRules, "hips_collision");
	RightArmCollisionBox->AttachToComponent(GetMesh(), AttachmentRules, "upperarm_r_collision");
	RightForearmCollisionBox->AttachToComponent(GetMesh(), AttachmentRules, "forearm_r_collision");
	LeftArmCollisionBox->AttachToComponent(GetMesh(), AttachmentRules, "upperarm_l_collision");
	LeftForearmCollisionBox->AttachToComponent(GetMesh(), AttachmentRules, "forearm_l_collision");
	RightThighCollisionBox->AttachToComponent(GetMesh(), AttachmentRules, "thigh_r_collision");
	RightLegCollisionBox->AttachToComponent(GetMesh(), AttachmentRules, "leg_r_collsion");
	LeftThighCollisionBox->AttachToComponent(GetMesh(), AttachmentRules, "thigh_l_collision");
	LeftLegCollisionBox->AttachToComponent(GetMesh(), AttachmentRules, "leg_l_collsion");
}

void AFightingCharacter::VariablesInit()
{
	DamageCBCategories.push_back("head");
	DamageCBCategories.push_back("torso");
	DamageCBCategories.push_back("right_arm");
	DamageCBCategories.push_back("left_arm");
	DamageCBCategories.push_back("right_leg");
	DamageCBCategories.push_back("left_leg");
	
	float current_time = GetWorld()->GetTimeSeconds();

	for (FString cat : DamageCBCategories) {
		DamagePotential[cat] = 1.0;
		LastDamageTakenTime[cat] = current_time;
	}

	BaseDamage["head"] = 0.02;
	BaseDamage["torso"] = 0.01;
	BaseDamage["right_arm"] = 0.005;
	BaseDamage["left_arm"] = 0.005;
	BaseDamage["right_leg"] = 0.005;
	BaseDamage["left_leg"] = 0.005;
}

FVector AFightingCharacter::GetEnemyLocation() {
	FVector target_location;
	if(TargetEnemy != NULL) target_location = TargetEnemy->GetActorLocation();
	return target_location;
}

float get_random_float()
{
	static std::default_random_engine e;
	static std::uniform_real_distribution<> dis(0, 1); // rage 0 - 1
	return dis(e);
}
